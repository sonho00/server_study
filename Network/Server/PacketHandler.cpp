#include "PacketHandler.hpp"

#include <iostream>

#include "Network/IOCP/NetUtils.hpp"
#include "Network/IOCP/Session.hpp"

namespace PacketHandler {
bool HandleC2S_MOVE(Session* session, void* packet) {
	C2S_MOVE* movePacket = reinterpret_cast<C2S_MOVE*>(packet);
	std::cout << "Handling C2S_MOVE packet: x=" << movePacket->x
			  << ", y=" << movePacket->y << std::endl;
	if (!session->Broadcast(packet)) {
		NetUtils::PrintError("Failed to broadcast C2S_MOVE packet");
		return false;
	}
	return true;
}

bool HandleC2S_CHAT(Session* session, void* packet) {
	C2S_CHAT* chatPacket = reinterpret_cast<C2S_CHAT*>(packet);
	std::cout << "Handling C2S_CHAT packet: message=" << chatPacket->message
			  << std::endl;
	if (!session->Broadcast(packet)) {
		NetUtils::PrintError("Failed to broadcast C2S_CHAT packet");
		return false;
	}

	// 클라이언트에게 에코 메시지 보내기

	size_t availableData =
		(session->writeOv.writePos_ - session->writeOv.readPos_) &
		(session->writeOv.buffer_.GetSize() - 1);

	PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(packet);
	if (availableData + header->size >= session->writeOv.buffer_.GetSize()) {
		NetUtils::PrintError("Write buffer overflow");
		session->Close();
		return false;
	}

	memmove(session->writeOv.buffer_.GetBuffer() + session->writeOv.writePos_,
			packet, header->size);
	session->writeOv.writePos_ = (session->writeOv.writePos_ + header->size) &
								 (session->writeOv.buffer_.GetSize() - 1);

	if (!session->isSending_) {
		session->isSending_ = true;
		session->writeOv.ioType_ = IO_TYPE::SEND;
		session->writeOv.wsaBuf_.buf =
			session->writeOv.buffer_.GetBuffer() + session->writeOv.readPos_;
		session->writeOv.wsaBuf_.len = static_cast<ULONG>(
			(session->writeOv.writePos_ - session->writeOv.readPos_) &
			(session->writeOv.buffer_.GetSize() - 1));
		ZeroMemory(&session->writeOv.overlapped_, sizeof(OVERLAPPED));
		DWORD flags = 0;
		int sendResult =
			WSASend(session->socket_, &session->writeOv.wsaBuf_, 1, NULL, flags,
					&session->writeOv.overlapped_, NULL);
		if (sendResult == SOCKET_ERROR &&
			WSAGetLastError() != ERROR_IO_PENDING) {
			NetUtils::PrintError("WSASend failed");
			session->Close();
			return false;
		}
	}

	return true;
}

std::function<bool(Session*, void*)> handlers[static_cast<size_t>(
	C2S_PACKET_ID::CNT)] = {nullptr, HandleC2S_MOVE, HandleC2S_CHAT};

bool Execute(Session* session, void* packet) {
	return handlers[static_cast<size_t>(
		reinterpret_cast<PACKET_HEADER*>(packet)->id)](session, packet);
}
}  // namespace PacketHandler
