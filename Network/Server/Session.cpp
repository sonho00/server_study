#include "Session.hpp"

#include <WinSock2.h>

#include <cstring>

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"
#include "PacketHandler.hpp"

bool Session::RegisterRead() {
	readOv.ioType_ = IO_TYPE::RECV;
	readOv.wsaBuf_.buf = readOv.buffer_.GetBuffer() + readOv.writePos_;
	readOv.wsaBuf_.len = readOv.buffer_.GetSize() - readOv.writePos_;
	ZeroMemory(&readOv.overlapped_, sizeof(OVERLAPPED));
	DWORD flags = 0;
	int recvResult = WSARecv(socket_, &readOv.wsaBuf_, 1, NULL, &flags,
							 &readOv.overlapped_, NULL);

	LOG_DEBUG(
		"[Session:{}] Posted WSARecv - Overlapped: {} BufferPos:{}, "
		"BufferLen:{}",
		static_cast<void*>(this), static_cast<void*>(&readOv.overlapped_),
		readOv.writePos_, readOv.wsaBuf_.len);

	if (recvResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("WSARecv failed: {}", errorCode);
			Close();
			return false;
		}
	}

	return true;
}

bool Session::RegisterWrite() {
	// 이 함수는 mtx 락을 획득한 상태이고 isSending_가 true인 상태에서만
	// 호출되어야 합니다.
	writeOv.ioType_ = IO_TYPE::SEND;
	writeOv.wsaBuf_.buf = writeOv.buffer_.GetBuffer() + writeOv.readPos_;
	writeOv.wsaBuf_.len = writeOv.writePos_ - writeOv.readPos_;
	ZeroMemory(&writeOv.overlapped_, sizeof(OVERLAPPED));
	DWORD flags = 0;
	int sendResult = WSASend(socket_, &writeOv.wsaBuf_, 1, NULL, flags,
							 &writeOv.overlapped_, NULL);

	LOG_DEBUG(
		"[Session:{}] Posted WSASend - Overlapped: {} BufferPos:{}, "
		"BufferLen:{}",
		static_cast<void*>(this), static_cast<void*>(&writeOv.overlapped_),
		writeOv.readPos_, writeOv.wsaBuf_.len);

	if (sendResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("WSASend failed: {}", errorCode);
			Close();
			return false;
		}
	}

	return true;
}

bool Session::SendPacket(const char* packet) {
	std::lock_guard<std::mutex> lock(mtx);

	const PACKET_HEADER* header =
		reinterpret_cast<const PACKET_HEADER*>(packet);
	memcpy(writeOv.buffer_.GetBuffer() + writeOv.writePos_, packet,
		   header->size);
	writeOv.writePos_ += header->size;

	if (!isSending_) {
		isSending_ = true;

		if (!RegisterWrite()) {
			LOG_ERROR("Failed to post another write");
			return false;
		}
	}

	return true;
}

bool Session::OnRead(const DWORD bytesTransferred) {
	if (readOv.writePos_ - readOv.readPos_ + bytesTransferred >
		readOv.buffer_.GetSize()) {
		LOG_ERROR("Read buffer overflow detected");
		Close();
		return false;
	}

	readOv.writePos_ += bytesTransferred;

	while (true) {
		size_t availableData = readOv.writePos_ - readOv.readPos_;
		if (availableData < sizeof(PACKET_HEADER)) break;

		PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(
			readOv.buffer_.GetBuffer() + readOv.readPos_);

		if (header->size == 0 || header->size > readOv.buffer_.GetSize()) {
			LOG_ERROR("Invalid packet size: {}", header->size);
			Close();
			return false;
		}

		if (availableData < header->size) break;

		if (!PacketHandler::Execute(this, header)) {
			LOG_ERROR("Failed to handle packet with ID: {}",
					  static_cast<uint16_t>(header->id));
			Close();
			return false;
		}

		readOv.readPos_ += header->size;
	}

	if (readOv.readPos_ >= readOv.buffer_.GetSize()) {
		readOv.readPos_ -= readOv.buffer_.GetSize();
		readOv.writePos_ -= readOv.buffer_.GetSize();
	}

	if (!RegisterRead()) {
		LOG_ERROR("Failed to post another read");
		return false;
	}

	return true;
}

bool Session::OnWrite(const DWORD bytesTransferred) {
	// 이 함수가 호출될 때는 is sending_가 true인 상태입니다.

	std::lock_guard<std::mutex> lock(mtx);
	if (writeOv.writePos_ - writeOv.readPos_ + bytesTransferred >
		writeOv.buffer_.GetSize()) {
		LOG_ERROR("Write buffer overflow detected");
		Close();
		return false;
	}

	writeOv.readPos_ += bytesTransferred;

	if (writeOv.readPos_ >= writeOv.buffer_.GetSize()) {
		writeOv.readPos_ -= writeOv.buffer_.GetSize();
		writeOv.writePos_ -= writeOv.buffer_.GetSize();
	}

	if (writeOv.readPos_ == writeOv.writePos_) {
		isSending_ = false;
		return true;
	}

	if (!RegisterWrite()) {
		LOG_ERROR("Failed to post another write");
		return false;
	}

	return true;
}

bool Session::HandleIO(OverlappedEx* ovEx, const DWORD bytesTransferred) {
	switch (ovEx->ioType_) {
		case IO_TYPE::RECV:
			return OnRead(bytesTransferred);
		case IO_TYPE::SEND:
			return OnWrite(bytesTransferred);
		default:
			LOG_ERROR("Unknown IO type");
			return false;
	}
}

void Session::Close() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}
