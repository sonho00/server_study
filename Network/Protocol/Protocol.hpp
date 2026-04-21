#pragma once

#include <cstdint>

enum class PacketID : uint16_t {
	NONE,
	C2S_MOVE,
	S2C_MOVE,
	C2S_CHAT,
	S2C_CHAT,
	ID_MAX
};

#pragma pack(push, 1)

struct PacketHeader {
	uint16_t size;
	PacketID id;
};

struct MovePacket {
	PacketHeader header;
	float x;
	float y;
};

struct ChatPacket {
	PacketHeader header;
	uint16_t len;
	wchar_t message[256];
};

#pragma pack(pop)
