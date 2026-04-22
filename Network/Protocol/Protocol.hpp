#pragma once

#include <cstdint>

enum class IO_TYPE { NONE, RECV, SEND, ACCEPT, CNT };
enum class C2S_PACKET_ID : uint16_t { NONE, MOVE, CHAT, CNT };
enum class S2C_PACKET_ID : uint16_t { NONE, MOVE, CHAT, CNT };

#pragma pack(push, 1)

struct PACKET_HEADER {
	uint16_t size;
	C2S_PACKET_ID id;
};

struct PACKET {
	PACKET_HEADER header;
};

struct C2S_MOVE : PACKET {
	float x;
	float y;
};

struct S2C_MOVE : PACKET {
	float x;
	float y;
	int sessionId;
};

struct C2S_CHAT : PACKET {
	char message[256];
};

struct S2C_CHAT : PACKET {
	char message[256];
	int sessionId;
};

#pragma pack(pop)
