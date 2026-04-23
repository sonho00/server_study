#include "Session.hpp"

#include <WinSock2.h>

#include <cstring>
#include <iostream>
#include <string>

#include "Network/Common/NetUtils.hpp"
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

	if (recvResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			NetUtils::PrintError("WSARecv failed", errorCode);
			Close();
			return false;
		}
	}

	return true;
}

bool Session::RegisterWrite(void* packet, size_t packetSize) {
	if (writeOv.writePos_ - writeOv.readPos_ + packetSize >
		writeOv.buffer_.GetSize()) {
		NetUtils::PrintError("Write buffer overflow");
		return false;
	}

	memcpy(writeOv.buffer_.GetBuffer() + writeOv.writePos_, packet, packetSize);
	writeOv.writePos_ += packetSize;

	if (isSending_) return true;
	isSending_ = true;

	writeOv.ioType_ = IO_TYPE::SEND;
	writeOv.wsaBuf_.buf = writeOv.buffer_.GetBuffer() + writeOv.readPos_;
	writeOv.wsaBuf_.len =
		static_cast<ULONG>(writeOv.writePos_ - writeOv.readPos_);
	ZeroMemory(&writeOv.overlapped_, sizeof(OVERLAPPED));
	DWORD flags = 0;
	int sendResult = WSASend(socket_, &writeOv.wsaBuf_, 1, NULL, flags,
							 &writeOv.overlapped_, NULL);

	if (sendResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			NetUtils::PrintError("WSASend failed", errorCode);
			Close();
			return false;
		}
	}

	return true;
}

bool Session::OnRead(DWORD bytesTransferred) {
	std::cout << "Session " << socket_ << " received " << bytesTransferred
			  << " bytes." << std::endl;

	readOv.writePos_ += bytesTransferred;

	while (true) {
		size_t avilableData = readOv.writePos_ - readOv.readPos_;
		if (avilableData < sizeof(PACKET_HEADER)) break;

		PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(
			readOv.buffer_.GetBuffer() + readOv.readPos_);

		if (header->size == 0 || header->size > readOv.buffer_.GetSize()) {
			NetUtils::PrintError("Invalid packet size");
			Close();
			return false;
		}

		if (avilableData < header->size) break;

		if (!PacketHandler::Execute(this, header)) {
			NetUtils::PrintError(
				("Failed to handle packet with ID: " +
				 std::to_string(static_cast<uint16_t>(header->id)))
					.c_str());
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
		NetUtils::PrintError("Failed to post another read");
		return false;
	}

	return true;
}

bool Session::OnWrite(DWORD bytesTransferred) {
	std::cout << "Session " << socket_ << " sent " << bytesTransferred
			  << " bytes." << std::endl;

	writeOv.readPos_ += bytesTransferred;

	if (writeOv.readPos_ >= writeOv.buffer_.GetSize()) {
		writeOv.readPos_ -= writeOv.buffer_.GetSize();
		writeOv.writePos_ -= writeOv.buffer_.GetSize();
	}

	size_t availableData = writeOv.writePos_ - writeOv.readPos_;
	if (availableData == 0) {
		isSending_ = false;
		return true;
	}

	if (!RegisterWrite(writeOv.buffer_.GetBuffer() + writeOv.readPos_,
					   availableData)) {
		NetUtils::PrintError("Failed to post another write");
		return false;
	}

	return true;
}

bool Session::Broadcast(void* packet) { return true; }

void Session::Close() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}
