#pragma once

#include <functional>

#include "Network/Common/Protocol.hpp"

class Session;

namespace PacketHandler {
extern std::function<bool(Session*, PACKET_HEADER*)>
	handlers[static_cast<size_t>(C2S_PACKET_ID::CNT)];

struct PacketRegistration {
	PacketRegistration(C2S_PACKET_ID id,
					   std::function<bool(Session*, PACKET_HEADER*)> handler) {
		handlers[static_cast<size_t>(id)] = handler;
	}
};

bool HandleC2S_MOVE(Session* session, PACKET_HEADER* packet);
bool HandleC2S_CHAT(Session* session, PACKET_HEADER* packet);
bool Execute(Session* session, PACKET_HEADER* packet);
}  // namespace PacketHandler

#define REGISTER_PACKET_HANDLER(id, handler)                             \
	static PacketHandler::PacketRegistration reg_##id(C2S_PACKET_ID::id, \
													  handler)
