#include "Session.hpp"

#include <WinSock2.h>
#include <minwinbase.h>
#include <minwindef.h>

#include <cstring>
#include <iostream>
#include <string>

#include "NetUtils.hpp"
#include "OverlappedEx.hpp"
#include "Protocol.hpp"

bool Session::OnRead(DWORD bytesTransferred) {
	readOv.writePos_ += bytesTransferred;

	while (true) {
		if (readOv.writePos_ - readOv.readPos_ < sizeof(PACKET_HEADER)) break;

		PACKET_HEADER* header =
			reinterpret_cast<PACKET_HEADER*>(readOv.buffer_ + readOv.readPos_);

		if (readOv.readPos_ + header->size > sizeof(readOv.buffer_) ||
			readOv.writePos_ == sizeof(readOv.buffer_)) {
			if (readOv.readPos_ == 0) {
				NetUtils::PrintError(
					"Client sent a packet that is too large to handle");
				Close();
				return false;
			}

			memmove(readOv.buffer_, readOv.buffer_ + readOv.readPos_,
					readOv.writePos_ - readOv.readPos_);
			readOv.writePos_ -= readOv.readPos_;
			readOv.readPos_ = 0;
		}

		if (readOv.writePos_ - readOv.readPos_ < header->size) break;

		if (!Handlers[static_cast<size_t>(header->id)]()) {
			std::cout << header->size << std::endl;
			NetUtils::PrintError(
				("Failed to handle packet with ID: " +
				 std::to_string(static_cast<uint16_t>(header->id)))
					.c_str());
			Close();
			return false;
		}

		std::cout << "Received message from client: "
				  << std::string(readOv.buffer_ + readOv.readPos_, header->size)
				  << std::endl;

		readOv.readPos_ += header->size;
	}

	readOv.wsaBuf_.buf = readOv.buffer_ + readOv.writePos_;
	readOv.wsaBuf_.len = sizeof(readOv.buffer_) - readOv.writePos_;
	ZeroMemory(&readOv.overlapped_, sizeof(OVERLAPPED));
	DWORD flags = 0;
	int recvResult = WSARecv(socket_, &readOv.wsaBuf_, 1, NULL, &flags,
							 &readOv.overlapped_, NULL);

	// WSARecv가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
	// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
	if (recvResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSARecv failed");
		Close();
		return false;
	}

	return true;
}

bool Session::OnWrite(DWORD bytesTransferred) {
	std::cout << "Sent message to client: "
			  << std::string(writeOv.buffer_, bytesTransferred) << std::endl;

	writeOv.readPos_ += bytesTransferred;

	if (writeOv.writePos_ < sizeof(PACKET_HEADER)) {
		isSending_ = false;
		return true;
	}

	PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(writeOv.buffer_);

	if (writeOv.readPos_ < header->size) {
		writeOv.wsaBuf_.buf = writeOv.buffer_ + writeOv.readPos_;
		writeOv.wsaBuf_.len = header->size - writeOv.readPos_;

		writeOv.taskType_ = Task::SEND;
		ZeroMemory(&writeOv.overlapped_, sizeof(OVERLAPPED));
		DWORD flags = 0;
		int sendResult = WSASend(socket_, &writeOv.wsaBuf_, 1, NULL, flags,
								 &writeOv.overlapped_, NULL);

		if (sendResult == SOCKET_ERROR &&
			WSAGetLastError() != ERROR_IO_PENDING) {
			NetUtils::PrintError("WSASend failed");
			Close();
			return false;
		}
		return true;
	}

	memmove(writeOv.buffer_, writeOv.buffer_ + writeOv.readPos_,
			writeOv.writePos_ - writeOv.readPos_);
	writeOv.writePos_ -= writeOv.readPos_;
	writeOv.readPos_ = 0;

	if (writeOv.writePos_ == 0 || writeOv.writePos_ < sizeof(PACKET_HEADER) ||
		writeOv.writePos_ < header->size) {
		isSending_ = false;
		return true;
	}

	writeOv.taskType_ = Task::SEND;
	writeOv.wsaBuf_.buf = writeOv.buffer_;
	writeOv.wsaBuf_.len = header->size;
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

bool Session::HandleIO(OverlappedEx<kReadBufferSize>* ovEx,
					   DWORD bytesTransferred) {
	switch (ovEx->taskType_) {
		case Task::RECV:
			return OnRead(bytesTransferred);
		case Task::SEND:
			return OnWrite(bytesTransferred);
		default:
			return false;
	}
}

bool Session::HandleC2S_MOVE(C2S_MOVE* packet) {
	std::cout << "Handling C2S_MOVE packet: x=" << packet->x
			  << ", y=" << packet->y << std::endl;
	if (!Broadcast(packet)) {
		NetUtils::PrintError("Failed to broadcast C2S_MOVE packet");
		return false;
	}
	return true;
}

bool Session::HandleC2S_CHAT(C2S_CHAT* packet) {
	std::cout << "Handling C2S_CHAT packet: message=" << packet->message
			  << std::endl;
	if (!Broadcast(packet)) {
		NetUtils::PrintError("Failed to broadcast C2S_CHAT packet");
		return false;
	}

	memmove(writeOv.buffer_ + writeOv.writePos_, packet, packet->header.size);
	writeOv.writePos_ += packet->header.size;

	if (!isSending_) {
		isSending_ = true;
		writeOv.taskType_ = Task::SEND;
		writeOv.wsaBuf_.buf = writeOv.buffer_ + writeOv.readPos_;
		writeOv.wsaBuf_.len = writeOv.writePos_ - writeOv.readPos_;
		ZeroMemory(&writeOv.overlapped_, sizeof(OVERLAPPED));
		DWORD flags = 0;
		int sendResult = WSASend(socket_, &writeOv.wsaBuf_, 1, NULL, flags,
								 &writeOv.overlapped_, NULL);

		if (sendResult == SOCKET_ERROR &&
			WSAGetLastError() != ERROR_IO_PENDING) {
			NetUtils::PrintError("WSASend failed");
			Close();
			return false;
		}
	}

	return true;
}

bool Session::Broadcast(PACKET* packet) { return true; }

void Session::Close() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}
