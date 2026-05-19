#include "Session.hpp"

#include <WinSock2.h>

#include <cassert>
#include <cstring>
#include <mutex>

#include "Network/Common/Logger.hpp"
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
		LOG_WARN("[Session:{}] Read buffer overflow detected", handle_);
		return false;
	}

	readOv_.ioType_ = IO_TYPE::kRecv;
	readOv_.wsaBuf_.buf = readOv_.buffer_.GetBuffer() + readOv_.writePos_;
	ZeroMemory(&readOv_.overlapped_, sizeof(OVERLAPPED));
	readOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD flags = 0;
	int result = WSARecv(socket_, &readOv_.wsaBuf_, 1, nullptr, &flags,
						 &readOv_.overlapped_, nullptr);

	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		readOv_.sessionPtr_.Reset();
		switch (errorCode) {
			case WSAECONNRESET:
				LOG_INFO("[Session:{}] Connection closed by client", handle_);
				return false;

			default:
				LOG_ERROR("[Session:{}][Error:{}] Failed to post read", handle_,
						  errorCode);
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

		if (header->size < sizeof(PACKET_HEADER) ||
			header->size >= readOv_.buffer_.GetSize()) {
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

bool Session::RegisterWriteInternal() {
	assert(isSending_);

	writeOv_.ioType_ = IO_TYPE::kSend;
	writeOv_.wsaBuf_.buf = writeOv_.buffer_.GetBuffer() + writeOv_.readPos_;
	writeOv_.wsaBuf_.len = writeOv_.writePos_ - writeOv_.readPos_;
	ZeroMemory(&writeOv_.overlapped_, sizeof(OVERLAPPED));
	writeOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD flags = 0;
	int result = WSASend(socket_, &writeOv_.wsaBuf_, 1, nullptr, flags,
						 &writeOv_.overlapped_, nullptr);

	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		writeOv_.sessionPtr_.Reset();
		switch (errorCode) {
			case WSAECONNRESET:
				LOG_INFO("[Session:{}] Connection closed by client", handle_);
				return false;

			default:
				LOG_ERROR("[Session:{}][Error:{}] Failed to post write",
						  handle_, errorCode);
				return false;
		}
	}
	return true;
}

bool Session::OnWrite(DWORD bytesTransferred) {
	std::lock_guard<std::mutex> lock(writeMtx_);
	assert(isSending_);

	writeOv_.readPos_ += bytesTransferred;

	if (writeOv_.readPos_ >= writeOv_.buffer_.GetSize()) {
		writeOv_.readPos_ -= writeOv_.buffer_.GetSize();
		writeOv_.writePos_ -= writeOv_.buffer_.GetSize();
	}

	if (writeOv_.readPos_ == writeOv_.writePos_) {
		isSending_ = false;
		return true;
	}

	if (!RegisterWriteInternal()) {
		LOG_ERROR("[Session:{}] Failed to post another write", handle_);
		return false;
	}

	return true;
}

bool Session::SendPacket(const PACKET_HEADER& header) {
	std::lock_guard<std::mutex> lock(writeMtx_);
	if (writeOv_.writePos_ - writeOv_.readPos_ + header.size >
		writeOv_.buffer_.GetSize()) {
		LOG_WARN("[Session:{}] Write buffer overflow detected", handle_);
		return false;
	}

	memcpy(writeOv_.buffer_.GetBuffer() + writeOv_.writePos_, &header,
		   header.size);
	writeOv_.writePos_ += header.size;

	if (!isSending_) {
		isSending_ = true;

		if (!RegisterWriteInternal()) {
			LOG_ERROR("[Session:{}] Failed to post another write", handle_);
			return false;
		}
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

bool Session::Connect() {
	{
		std::lock_guard<std::mutex> lock(connectMtx_);
		if (sessionManager_->GetState(handle_) != SessionState::kPending) {
			LOG_ERROR("[Session:{}] Invalid state for Connect: {}", handle_,
					  static_cast<uint8_t>(sessionManager_->GetState(handle_)));
			return false;
		}

		if (!sessionManager_->ConnectSession(handle_)) {
			LOG_ERROR("[Session:{}] Failed to transition to Connected state",
					  handle_);
			return false;
		}
	}

	S2C_CHAT welcomePacket{};
	welcomePacket.header.id = static_cast<uint16_t>(S2C_PACKET_ID::kChat);
	sprintf(welcomePacket.message, "%lld", handle_);
	welcomePacket.header.size =
		sizeof(welcomePacket.header) + strlen(welcomePacket.message) + 1;

	if (!SendPacket(welcomePacket.header)) {
		LOG_ERROR("[Session:{}] Failed to send welcome packet", handle_);
		return false;
	}

	if (!RegisterRead()) {
		LOG_WARN("[Session:{}] Failed to post initial read", handle_);
		return false;
	}

	return true;
}

bool Session::Disconnect() {
	{
		std::lock_guard<std::mutex> lock(connectMtx_);
		switch (sessionManager_->GetState(handle_)) {
			case SessionState::kPending:
			case SessionState::kConnected:
				if (!sessionManager_->SetState(handle_,
											   SessionState::kDisconnecting)) {
					LOG_ERROR(
						"[Session:{}] Failed to transition to Disconnecting "
						"state",
						handle_);
					return false;
				}
				break;

			case SessionState::kDisconnecting:
				LOG_WARN("[Session:{}] Already in Disconnecting state",
						 handle_);
				return true;

			default:
				LOG_ERROR(
					"[Session:{}] Invalid state for Disconnect: {}", handle_,
					static_cast<uint8_t>(sessionManager_->GetState(handle_)));
				return false;
		}
	}

	disconnectOv_.ioType_ = IO_TYPE::kDisconnect;
	ZeroMemory(&disconnectOv_.overlapped_, sizeof(OVERLAPPED));
	disconnectOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	int result = ServerUtils::DisconnectEx(socket_, &disconnectOv_.overlapped_,
										   TF_REUSE_SOCKET, 0);
	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		LOG_ERROR("[Session:{}][Error:{}] Failed to post disconnect", handle_,
				  errorCode);
		switch (errorCode) {
			default:
				LOG_ERROR("[Session:{}][Error:{}] Failed to post disconnect",
						  handle_, errorCode);
				break;
		}
		return false;
	}
	return true;
}

bool Session::Clear() {
	sessionManager_->DisconnectSession(handle_);
	return true;
}
