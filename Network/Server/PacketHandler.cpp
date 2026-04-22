#include "PacketHandler.hpp"

#include <iostream>

#include "Network/Common/NetUtils.hpp"
#include "Session.hpp"

namespace PacketHandler {
bool HandleC2S_MOVE(Session* session, void* packet) {
	C2S_MOVE* movePacket = reinterpret_cast<C2S_MOVE*>(packet);
	std::cout << "Session " << session->socket_
			  << " handling C2S_MOVE packet: x=" << movePacket->x
			  << " y=" << movePacket->y << std::endl;
	if (!session->Broadcast(packet)) {
		NetUtils::PrintError("Failed to broadcast C2S_MOVE packet");
		return false;
	}

	if (!session->RegisterWrite(packet, movePacket->header.size)) {
		NetUtils::PrintError("Failed to post write for C2S_MOVE packet");
		session->Close();
		return false;
	}
	return true;
}

bool HandleC2S_CHAT(Session* session, void* packet) {
	C2S_CHAT* chatPacket = reinterpret_cast<C2S_CHAT*>(packet);
	std::cout << "Session " << session->socket_
			  << " handling C2S_CHAT packet: message=" << chatPacket->message
			  << std::endl;
	if (!session->Broadcast(packet)) {
		NetUtils::PrintError("Failed to broadcast C2S_CHAT packet");
		return false;
	}

	if (!session->RegisterWrite(packet, chatPacket->header.size)) {
		NetUtils::PrintError("Failed to post write for C2S_CHAT packet");
		session->Close();
		return false;
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
