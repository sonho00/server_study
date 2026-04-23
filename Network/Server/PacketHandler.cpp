#include "PacketHandler.hpp"

#include <iostream>

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/Protocol.hpp"
#include "Session.hpp"

namespace PacketHandler {
std::function<bool(Session*, PACKET_HEADER*)>
	handlers[static_cast<size_t>(C2S_PACKET_ID::CNT)];

bool HandleC2S_MOVE(Session* session, PACKET_HEADER* packet) {
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
REGISTER_PACKET_HANDLER(MOVE, HandleC2S_MOVE);

bool HandleC2S_CHAT(Session* session, PACKET_HEADER* packet) {
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
REGISTER_PACKET_HANDLER(CHAT, HandleC2S_CHAT);

bool Execute(Session* session, PACKET_HEADER* packet) {
	return handlers[static_cast<size_t>(packet->id)](session, packet);
}
}  // namespace PacketHandler
