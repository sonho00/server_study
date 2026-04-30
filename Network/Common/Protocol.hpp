#pragma once

#include <cstdint>

#include "Config.hpp"

enum class IO_TYPE : uint8_t { kNone, kRecv, kSend, kAccept, kCnt };
enum class C2S_PACKET_ID : uint8_t { kNone, kMove, kChat, kCnt };
enum class S2C_PACKET_ID : uint8_t { kNone, kMove, kChat, kCnt };

// NOLINTBEGIN(readability-identifier-naming, modernize-avoid-c-arrays)
#pragma pack(push, 1)

struct PACKET_HEADER {
	int size;
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
	uint16_t sessionId;
};

struct C2S_CHAT {
	PACKET_HEADER header;
	char message[Config::kChatPacketSize];
};

struct S2C_CHAT {
	PACKET_HEADER header;
	char message[Config::kChatPacketSize];
	uint16_t sessionId;
};

#pragma pack(pop)
// NOLINTEND(readability-identifier-naming, modernize-avoid-c-arrays)
