#include "Session.hpp"

#include <WinSock2.h>

#include <cstring>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"
#include "PacketHandler.hpp"
#include "SessionManager.hpp"

bool Session::RegisterRead() {
	readOv_.ioType_ = IO_TYPE::kRecv;
	readOv_.wsaBuf_.buf = readOv_.buffer_.GetBuffer() + readOv_.writePos_;
	readOv_.wsaBuf_.len =
		readOv_.buffer_.GetSize() - readOv_.writePos_ + readOv_.readPos_;
	ZeroMemory(&readOv_.overlapped_, sizeof(OVERLAPPED));

	readOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	if (readOv_.wsaBuf_.len == 0) {
		LOG_ERROR("[Session:{}] Read buffer overflow detected", handle_);
		Close();
		return false;
	}
	readOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD flags = 0;
	int recvResult = WSARecv(socket_, &readOv_.wsaBuf_, 1, nullptr, &flags,
							 &readOv_.overlapped_, nullptr);

	if (recvResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("[Session:{}] WSARecv failed: {}", handle_, errorCode);
			Close();
			return false;
		}
	}

	return true;
}

bool Session::RegisterWrite() {
	// 이 함수는 mtx 락을 획득한 상태이고 isSending_가 true인 상태에서만
	// 호출되어야 합니다.
	writeOv_.ioType_ = IO_TYPE::kSend;
	writeOv_.wsaBuf_.buf = writeOv_.buffer_.GetBuffer() + writeOv_.readPos_;
	writeOv_.wsaBuf_.len = writeOv_.writePos_ - writeOv_.readPos_;
	ZeroMemory(&writeOv_.overlapped_, sizeof(OVERLAPPED));

	writeOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD flags = 0;
	int sendResult = WSASend(socket_, &writeOv_.wsaBuf_, 1, nullptr, flags,
							 &writeOv_.overlapped_, nullptr);

	if (sendResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("[Session:{}] WSASend failed: {}", handle_, errorCode);
			Close();
			return false;
		}
	}

	return true;
}

bool Session::SendPacket(const PACKET_HEADER& header) {
	std::lock_guard<std::mutex> lock(mtx_);
	if (writeOv_.writePos_ - writeOv_.readPos_ + header.size >
		writeOv_.buffer_.GetSize()) {
		LOG_ERROR("[Session:{}] Write buffer overflow detected", handle_);
		Close();
		return false;
	}

	memcpy(writeOv_.buffer_.GetBuffer() + writeOv_.writePos_, &header,
		   header.size);
	writeOv_.writePos_ += header.size;

	if (!isSending_) {
		isSending_ = true;

		if (!RegisterWrite()) {
			LOG_ERROR("[Session:{}] Failed to post another write", handle_);
			return false;
		}
	}

	return true;
}

bool Session::OnRead(DWORD bytesTransferred) {
	readOv_.writePos_ += bytesTransferred;

	while (true) {
		size_t availableData = readOv_.writePos_ - readOv_.readPos_;
		if (availableData < sizeof(PACKET_HEADER)) break;

		auto* header = reinterpret_cast<PACKET_HEADER*>(
			readOv_.buffer_.GetBuffer() + readOv_.readPos_);

		if (header->size == 0 || header->size > readOv_.buffer_.GetSize()) {
			LOG_ERROR("[Session:{}] Invalid packet size: {}", handle_,
					  header->size);
			Close();
			return false;
		}

		if (availableData < header->size) break;

		if (!PacketHandler::Execute(*this, *header)) {
			LOG_ERROR("[Session:{}] Failed to handle packet with ID: {}",
					  handle_, static_cast<uint16_t>(header->id));
			Close();
			return false;
		}

		LOG_DEBUG("[Session:{}] Processed packet ID: {}, Size: {}", handle_,
				  static_cast<uint16_t>(header->id), header->size);

		readOv_.readPos_ += header->size;
	}

	if (readOv_.sessionPtr_.Reset()) {
		LOG_INFO("[Session:{}] Session reset after read completion", handle_);
		Close();
		return true;
	}

	if (readOv_.readPos_ >= readOv_.buffer_.GetSize()) {
		readOv_.readPos_ -= readOv_.buffer_.GetSize();
		readOv_.writePos_ -= readOv_.buffer_.GetSize();
	}

	if (!RegisterRead()) {
		LOG_ERROR("[Session:{}] Failed to post another read", handle_);
		return false;
	}

	return true;
}

bool Session::OnWrite(DWORD bytesTransferred) {
	// 이 함수가 호출될 때는 is sending_가 true인 상태입니다.
	std::lock_guard<std::mutex> lock(mtx_);
	writeOv_.readPos_ += bytesTransferred;

	if (writeOv_.sessionPtr_.Reset()) {
		LOG_INFO("[Session:{}] Session released after write completion",
				 handle_);
		return true;
	}

	if (writeOv_.readPos_ >= writeOv_.buffer_.GetSize()) {
		writeOv_.readPos_ -= writeOv_.buffer_.GetSize();
		writeOv_.writePos_ -= writeOv_.buffer_.GetSize();
	}

	if (writeOv_.readPos_ == writeOv_.writePos_) {
		isSending_ = false;
		return true;
	}

	if (!RegisterWrite()) {
		LOG_ERROR("[Session:{}] Failed to post another write", handle_);
		return false;
	}

	return true;
}

bool Session::HandleIO(OverlappedEx& ovEx, DWORD bytesTransferred) {
	switch (ovEx.ioType_) {
		case IO_TYPE::kRecv:
			return OnRead(bytesTransferred);
		case IO_TYPE::kSend:
			return OnWrite(bytesTransferred);
		default:
			LOG_ERROR("[Session:{}] Unknown IO type", handle_);
			return false;
	}
}

void Session::Close() {
	if (socket_ != INVALID_SOCKET) {
		LOG_INFO("[Session:{}] Socket closed", handle_);
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

void Session::Reset() {
	std::lock_guard<std::mutex> lock(mtx_);
	readOv_.Reset();
	writeOv_.Reset();
	socket_ = INVALID_SOCKET;
	isSending_ = false;
}
