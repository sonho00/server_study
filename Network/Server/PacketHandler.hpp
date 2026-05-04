#pragma once

#include <array>
#include <functional>

#include "Network/Common/Protocol.hpp"

class Session;
using HandlerFunc = std::function<bool(Session&, const PACKET_HEADER&)>;

namespace PacketHandler {
extern std::array<HandlerFunc, static_cast<size_t>(C2S_PACKET_ID::kCnt)>
	handlers;

struct PacketRegistration {
	PacketRegistration(C2S_PACKET_ID packetId, HandlerFunc handler) {
		handlers[static_cast<size_t>(packetId)] = std::move(handler);
	}
};

bool HandleC2S_MOVE(Session& session, const PACKET_HEADER& header);
bool HandleC2S_CHAT(Session& session, const PACKET_HEADER& header);
bool Execute(Session& session, const PACKET_HEADER& header);
}  // namespace PacketHandler

#define REGISTER_PACKET_HANDLER(id, handler)                             \
	static PacketHandler::PacketRegistration reg_##id(C2S_PACKET_ID::id, \
													  handler)
