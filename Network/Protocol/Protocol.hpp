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

struct C2S_MOVE {
	PacketHeader header;
	float x;
	float y;
};

struct S2C_MOVE {
	PacketHeader header;
	int sessionId;
	float x;
	float y;
};

struct C2S_CHAT {
	PacketHeader header;
	char message[256];
};

struct S2C_CHAT {
	PacketHeader header;
	int sessionId;
	char message[256];
};

#pragma pack(pop)
