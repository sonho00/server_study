#include "PacketHandler.hpp"

#include <cstring>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Session.hpp"
#include "SessionManager.hpp"

namespace PacketHandler {
std::array<HandlerFunc, static_cast<size_t>(C2S_PACKET_ID::kCnt)> handlers;

bool HandleC2S_MOVE(SessionManager& sessionManager, Session& session,
					const PACKET_HEADER& header) {
	const auto* moveData = reinterpret_cast<const C2S_MOVE*>(&header);
	S2C_MOVE movePacket{};
	movePacket.header.id = static_cast<uint16_t>(S2C_PACKET_ID::kMove);
	movePacket.header.size = sizeof(S2C_MOVE);
	movePacket.sessionHandle = session.GetHandle();
	movePacket.x = moveData->x;
	movePacket.y = moveData->y;

	if (!sessionManager.Broadcast(
			reinterpret_cast<const PACKET_HEADER&>(movePacket),
			session.GetHandle())) {
		LOG_ERROR("Failed to broadcast MOVE packet");
		return false;
	}
	return true;
}
REGISTER_PACKET_HANDLER(kMove, [](Session& session,
								  const PACKET_HEADER& header) {
	return HandleC2S_MOVE(*session.GetSessionManager(), session, header);
});

bool HandleC2S_CHAT(SessionManager& sessionManager, Session& session,
					const PACKET_HEADER& header) {
	std::vector<char> data(header.size + sizeof(session.GetHandle()));
	auto* chatPacket = reinterpret_cast<S2C_CHAT*>(data.data());
	chatPacket->header.id = static_cast<uint16_t>(S2C_PACKET_ID::kChat);
	chatPacket->header.size = header.size + sizeof(chatPacket->sessionHandle);
	chatPacket->sessionHandle = session.GetHandle();
	std::memcpy(chatPacket->message,
				reinterpret_cast<const char*>(&header) + sizeof(PACKET_HEADER),
				header.size - sizeof(PACKET_HEADER));

	if (!sessionManager.Broadcast(
			reinterpret_cast<const PACKET_HEADER&>(*chatPacket),
			session.GetHandle())) {
		LOG_ERROR("Failed to broadcast CHAT packet");
		return false;
	}

	return true;
}
REGISTER_PACKET_HANDLER(kChat, [](Session& session,
								  const PACKET_HEADER& header) {
	return HandleC2S_CHAT(*session.GetSessionManager(), session, header);
});

bool Execute(Session& session, const PACKET_HEADER& header) {
	return handlers[static_cast<size_t>(header.id)](session, header);
}
}  // namespace PacketHandler
