#include "Session.hpp"

#include <WinSock2.h>

#include <cstring>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"
#include "PacketHandler.hpp"

bool Session::RegisterRead() {
	readOv_.ioType_ = IO_TYPE::kRecv;
	readOv_.wsaBuf_.buf = readOv_.buffer_.GetBuffer() + readOv_.writePos_;
	readOv_.wsaBuf_.len =
		readOv_.buffer_.GetSize() - readOv_.writePos_ + readOv_.readPos_;
	ZeroMemory(&readOv_.overlapped_, sizeof(OVERLAPPED));

	DWORD flags = 0;
	int recvResult = WSARecv(socket_, &readOv_.wsaBuf_, 1, nullptr, &flags,
							 &readOv_.overlapped_, nullptr);

	LOG_DEBUG(
		"[Session:{}] Posted WSARecv - Overlapped: {} BufferPos:{}, "
		"BufferLen:{}",
		GetHandle(), static_cast<void*>(&readOv_.overlapped_),
		readOv_.writePos_, readOv_.wsaBuf_.len);

	if (recvResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("[Session:{}] WSARecv failed: {}", GetHandle(),
					  errorCode);
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

	DWORD flags = 0;
	int sendResult = WSASend(socket_, &writeOv_.wsaBuf_, 1, nullptr, flags,
							 &writeOv_.overlapped_, nullptr);

	LOG_DEBUG(
		"[Session:{}] Posted WSASend - Overlapped: {} BufferPos:{}, "
		"BufferLen:{}",
		GetHandle(), static_cast<void*>(&writeOv_.overlapped_),
		writeOv_.readPos_, writeOv_.wsaBuf_.len);

	if (sendResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("[Session:{}] WSASend failed: {}", GetHandle(),
					  errorCode);
			Close();
			return false;
		}
	}

	return true;
}

bool Session::SendPacket(const PACKET_HEADER& header) {
	std::lock_guard<std::mutex> lock(mtx_);

	memcpy(writeOv_.buffer_.GetBuffer() + writeOv_.writePos_, &header,
		   header.size);
	writeOv_.writePos_ += header.size;

	if (!isSending_) {
		isSending_ = true;

		if (!RegisterWrite()) {
			LOG_ERROR("[Session:{}] Failed to post another write", GetHandle());
			return false;
		}
	}

	return true;
}

bool Session::OnRead(DWORD bytesTransferred) {
	if (readOv_.writePos_ - readOv_.readPos_ + bytesTransferred >
		readOv_.buffer_.GetSize()) {
		LOG_ERROR("[Session:{}] Read buffer overflow detected", GetHandle());
		Close();
		return false;
	}

	readOv_.writePos_ += bytesTransferred;

	while (true) {
		size_t availableData = readOv_.writePos_ - readOv_.readPos_;
		if (availableData < sizeof(PACKET_HEADER)) break;

		auto* header = reinterpret_cast<PACKET_HEADER*>(
			readOv_.buffer_.GetBuffer() + readOv_.readPos_);

		if (header->size == 0 || header->size > readOv_.buffer_.GetSize()) {
			LOG_ERROR("[Session:{}] Invalid packet size: {}", GetHandle(),
					  header->size);
			Close();
			return false;
		}

		if (availableData < header->size) break;

		if (!PacketHandler::Execute(*this, *header)) {
			LOG_ERROR("[Session:{}] Failed to handle packet with ID: {}",
					  GetHandle(), static_cast<uint16_t>(header->id));
			Close();
			return false;
		}

		LOG_DEBUG("[Session:{}] Processed packet ID: {}, Size: {}", GetHandle(),
				  static_cast<uint16_t>(header->id), header->size);

		readOv_.readPos_ += header->size;
	}

	if (readOv_.readPos_ >= readOv_.buffer_.GetSize()) {
		readOv_.readPos_ -= readOv_.buffer_.GetSize();
		readOv_.writePos_ -= readOv_.buffer_.GetSize();
	}

	if (!RegisterRead()) {
		LOG_ERROR("[Session:{}] Failed to post another read", GetHandle());
		return false;
	}

	return true;
}

bool Session::OnWrite(DWORD bytesTransferred) {
	// 이 함수가 호출될 때는 is sending_가 true인 상태입니다.

	std::lock_guard<std::mutex> lock(mtx_);
	writeOv_.readPos_ += bytesTransferred;

	if (writeOv_.writePos_ - writeOv_.readPos_ + bytesTransferred >
		writeOv_.buffer_.GetSize()) {
		LOG_ERROR("[Session:{}] Write buffer overflow detected", GetHandle());
		Close();
		return false;
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
		LOG_ERROR("[Session:{}] Failed to post another write", GetHandle());
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
			LOG_ERROR("[Session:{}] Unknown IO type", GetHandle());
			return false;
	}
}

void Session::Close() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

void Session::Reset() {
	LOG_DEBUG("[Session:{}] Resetting session", GetHandle());
	std::lock_guard<std::mutex> lock(mtx_);
	readOv_.Reset();
	writeOv_.Reset();
	socket_ = INVALID_SOCKET;
	isSending_ = false;
}