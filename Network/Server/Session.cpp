#include "Session.hpp"

#include <WinSock2.h>

#include <cstring>
#include <mutex>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Pool/ISparsePool.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"
#include "PacketHandler.hpp"
#include "ServerUtils.hpp"
#include "SessionManager.hpp"

Session::Session()
	: readOv_(Config::kMagicBufferSize), writeOv_(Config::kMagicBufferSize) {
	socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
						WSA_FLAG_OVERLAPPED);
	if (socket_ == INVALID_SOCKET) {
		LOG_ERROR("Failed to create accept socket");
	}
}

bool Session::RegisterRead() {
	readOv_.wsaBuf_.len =
		readOv_.buffer_.GetSize() - readOv_.writePos_ + readOv_.readPos_;

	if (readOv_.wsaBuf_.len == 0) {
		LOG_ERROR("[Session:{}] Read buffer overflow detected", handle_);
		return false;
	}

	readOv_.ioType_ = IO_TYPE::kRecv;
	readOv_.wsaBuf_.buf = readOv_.buffer_.GetBuffer() + readOv_.writePos_;
	ZeroMemory(&readOv_.overlapped_, sizeof(OVERLAPPED));
	readOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD flags = 0;
	int recvResult = WSARecv(socket_, &readOv_.wsaBuf_, 1, nullptr, &flags,
							 &readOv_.overlapped_, nullptr);

	if (recvResult == SOCKET_ERROR) {
		ServerUtils::HandleError(readOv_.sessionPtr_, WSAGetLastError());
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
		ServerUtils::HandleError(writeOv_.sessionPtr_, WSAGetLastError());
	}

	return true;
}

bool Session::SendPacket(const PACKET_HEADER& header) {
	std::lock_guard<std::mutex> lock(mtx_);
	if (writeOv_.writePos_ - writeOv_.readPos_ + header.size >
		writeOv_.buffer_.GetSize()) {
		LOG_ERROR("[Session:{}] Write buffer overflow detected", handle_);
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
	if (readOv_.sessionPtr_.Reset()) {
		LOG_INFO("[Session:{}] Session reset after read completion", handle_);
		return true;
	}

	readOv_.writePos_ += bytesTransferred;

	while (true) {
		size_t availableData = readOv_.writePos_ - readOv_.readPos_;
		if (availableData < sizeof(PACKET_HEADER)) break;

		auto* header = reinterpret_cast<PACKET_HEADER*>(
			readOv_.buffer_.GetBuffer() + readOv_.readPos_);

		if (header->size == 0 || header->size > readOv_.buffer_.GetSize()) {
			LOG_ERROR("[Session:{}] Invalid packet size: {}", handle_,
					  header->size);
			return false;
		}

		if (availableData < header->size) break;

		if (!PacketHandler::Execute(*this, *header)) {
			LOG_ERROR("[Session:{}] Failed to handle packet with ID: {}",
					  handle_, static_cast<uint16_t>(header->id));
			return false;
		}

		LOG_DEBUG("[Session:{}] Processed packet ID: {}, Size: {}", handle_,
				  static_cast<uint16_t>(header->id), header->size);

		readOv_.readPos_ += header->size;
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
	if (writeOv_.sessionPtr_.Reset()) {
		LOG_INFO("[Session:{}] Session released after write completion",
				 handle_);
		return true;
	}

	std::lock_guard<std::mutex> lock(mtx_);
	writeOv_.readPos_ += bytesTransferred;

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

bool Session::Disconnect() {
	{
		std::lock_guard<std::mutex> lock(mtx_);

		if (sessionManager_->GetState(handle_) ==
			SessionState::kDisconnecting) {
			LOG_INFO("[Session:{}] Already disconnecting", handle_);
			return true;
		}
		sessionManager_->SetState(handle_, SessionState::kDisconnecting);
	}

	disconnectOv_.ioType_ = IO_TYPE::kDisconnect;
	ZeroMemory(&disconnectOv_.overlapped_, sizeof(OVERLAPPED));
	disconnectOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	if (ServerUtils::DisconnectEx(socket_, &disconnectOv_.overlapped_,
								  TF_REUSE_SOCKET, 0) == SOCKET_ERROR) {
		ServerUtils::HandleError(disconnectOv_.sessionPtr_, WSAGetLastError());
	}

	return true;
}

bool Session::Clear() {
	readOv_.Reset();
	writeOv_.Reset();
	disconnectOv_.Reset();
	isSending_ = false;
	sessionManager_->DisconnectSession(handle_);
	handle_ = ISparsePool<Session>::kInvalidHandle;
	return true;
}
