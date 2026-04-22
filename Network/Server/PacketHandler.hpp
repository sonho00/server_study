#pragma once

class Session;

namespace PacketHandler {
bool HandleC2S_MOVE(Session* session, void* packet);
bool HandleC2S_CHAT(Session* session, void* packet);
bool Execute(Session* session, void* packet);
}  // namespace PacketHandler
