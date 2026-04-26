#include "PacketHandler.hpp"

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/Protocol.hpp"
#include "Session.hpp"

namespace PacketHandler {
std::function<bool(Session*, const PACKET_HEADER*)>
	handlers[static_cast<size_t>(C2S_PACKET_ID::CNT)];

bool HandleC2S_MOVE(Session* session, const PACKET_HEADER* header) {
	const C2S_MOVE* movePacket = reinterpret_cast<const C2S_MOVE*>(header);

	if (!session->SendPacket(reinterpret_cast<const char*>(movePacket))) {
		LOG_ERROR("Failed to echo MOVE packet");
		session->Close();
		return false;
	}
	return true;
}
REGISTER_PACKET_HANDLER(MOVE, HandleC2S_MOVE);

bool HandleC2S_CHAT(Session* session, const PACKET_HEADER* header) {
	const C2S_CHAT* chatPacket = reinterpret_cast<const C2S_CHAT*>(header);

	if (!session->SendPacket(reinterpret_cast<const char*>(chatPacket))) {
		LOG_ERROR("Failed to echo CHAT packet");
		session->Close();
		return false;
	}

	return true;
}
REGISTER_PACKET_HANDLER(CHAT, HandleC2S_CHAT);

bool Execute(Session* session, const PACKET_HEADER* packet) {
	return handlers[static_cast<size_t>(packet->id)](session, packet);
}
}  // namespace PacketHandler
