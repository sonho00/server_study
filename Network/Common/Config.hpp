#pragma once

#include <WinSock2.h>

#include <cstdint>

namespace Config {
constexpr uint16_t kPort = 8080;
constexpr uint32_t kClientCount = 16;

constexpr uint32_t kPoolSize = 4096;
constexpr uint32_t kInitialAcceptCount = 16;

constexpr uint32_t kChatPacketSize = 256;
constexpr uint32_t kAcceptAddrSize = sizeof(sockaddr_in) + 16;

constexpr uint32_t kClientBufferSize = 65536;
constexpr uint32_t kMagicBufferSize = 65536;
}  // namespace Config
