#include "PacketHandler.hpp"

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Session.hpp"

namespace PacketHandler {
std::array<HandlerFunc, static_cast<size_t>(C2S_PACKET_ID::kCnt)> handlers;

bool HandleC2S_MOVE(Session& session, const PACKET_HEADER& header) {
	if (!session.SendPacket(header)) {
		LOG_ERROR("Failed to echo MOVE packet");
		session.Close();
		return false;
	}
	return true;
}
REGISTER_PACKET_HANDLER(kMove, HandleC2S_MOVE);

bool HandleC2S_CHAT(Session& session, const PACKET_HEADER& header) {
	if (!session.SendPacket(header)) {
		LOG_ERROR("Failed to echo CHAT packet");
		session.Close();
		return false;
	}

	return true;
}
REGISTER_PACKET_HANDLER(kChat, HandleC2S_CHAT);

bool Execute(Session& session, const PACKET_HEADER& header) {
	return handlers[static_cast<size_t>(header.id)](session, header);
}
}  // namespace PacketHandler
