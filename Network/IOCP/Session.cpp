#include "Session.hpp"

#include <WinSock2.h>

#include <cstring>
#include <iostream>
#include <string>

#include "NetUtils.hpp"
#include "Network/Server/PacketHandler.hpp"
#include "OverlappedEx.hpp"

bool Session::OnRead(DWORD bytesTransferred) {
	readOv.writePos_ =
		(readOv.writePos_ + bytesTransferred) & (readOv.buffer_.GetSize() - 1);

	while (true) {
		size_t avilableData = (readOv.writePos_ - readOv.readPos_) &
							  (readOv.buffer_.GetSize() - 1);
		if (avilableData < sizeof(PACKET_HEADER)) break;

		PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(
			readOv.buffer_.GetBuffer() + readOv.readPos_);

		if (header->size == 0 || header->size > readOv.buffer_.GetSize()) {
			NetUtils::PrintError("Invalid packet size");
			Close();
			return false;
		}

		if (avilableData < header->size) break;

		if (!PacketHandler::Execute(
				this, readOv.buffer_.GetBuffer() + readOv.readPos_)) {
			std::cout << header->size << std::endl;
			NetUtils::PrintError(
				("Failed to handle packet with ID: " +
				 std::to_string(static_cast<uint16_t>(header->id)))
					.c_str());
			Close();
			return false;
		}

		std::cout << "Received message from client: "
				  << std::string(readOv.buffer_.GetBuffer() + readOv.readPos_,
								 header->size)
				  << std::endl;

		readOv.readPos_ += header->size;
	}

	if (readOv.readPos_ == readOv.writePos_) {
		readOv.readPos_ = 0;
		readOv.writePos_ = 0;
	}

	readOv.wsaBuf_.buf = readOv.buffer_.GetBuffer() + readOv.writePos_;
	readOv.wsaBuf_.len = (writeOv.readPos_ - writeOv.writePos_ - 1) &
						 (writeOv.buffer_.GetSize() - 1);
	ZeroMemory(&readOv.overlapped_, sizeof(OVERLAPPED));
	DWORD flags = 0;
	int recvResult = WSARecv(socket_, &readOv.wsaBuf_, 1, NULL, &flags,
							 &readOv.overlapped_, NULL);

	if (recvResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSARecv failed");
		Close();
		return false;
	}

	return true;
}

bool Session::OnWrite(DWORD bytesTransferred) {
	std::cout << "Sent message to client: "
			  << std::string(writeOv.buffer_.GetBuffer(), bytesTransferred)
			  << std::endl;

	writeOv.readPos_ =
		(writeOv.readPos_ + bytesTransferred) & (writeOv.buffer_.GetSize() - 1);

	size_t availableData = (writeOv.writePos_ - writeOv.readPos_) &
						   (writeOv.buffer_.GetSize() - 1);

	if (availableData == 0) {
		isSending_ = false;
		return true;
	}

	writeOv.ioType_ = IO_TYPE::SEND;
	writeOv.wsaBuf_.buf = writeOv.buffer_.GetBuffer() + writeOv.readPos_;
	writeOv.wsaBuf_.len = static_cast<ULONG>(availableData);
	ZeroMemory(&writeOv.overlapped_, sizeof(OVERLAPPED));
	DWORD flags = 0;
	int sendResult = WSASend(socket_, &writeOv.wsaBuf_, 1, NULL, flags,
							 &writeOv.overlapped_, NULL);

	if (sendResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSASend failed");
		Close();
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
