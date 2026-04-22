#pragma once

#include <cstdint>

enum class IO_TYPE { NONE, RECV, SEND, ACCEPT, CNT };
enum class C2S_PACKET_ID : uint16_t { NONE, MOVE, CHAT, CNT };
enum class S2C_PACKET_ID : uint16_t { NONE, MOVE, CHAT, CNT };

#pragma pack(push, 1)

struct PACKET_HEADER {
	uint16_t size;
	uint16_t id;
};

struct C2S_MOVE {
	PACKET_HEADER header;
	float x;
	float y;
};

struct S2C_MOVE {
	PACKET_HEADER header;
	float x;
	float y;
	int sessionId;
};

struct C2S_CHAT {
	PACKET_HEADER header;
	char message[256];
};

struct S2C_CHAT {
	PACKET_HEADER header;
	char message[256];
	int sessionId;
};

#pragma pack(pop)
