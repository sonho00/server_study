#pragma once

#include <WinSock2.h>

#include <array>

#include "Network/Common/Protocol.hpp"

SOCKET CreateClientSocket(const char* ipAddr, uint16_t port);

class Client {
   public:
	Client(const char* ipAddr, uint16_t port);
	~Client();

	[[nodiscard]] bool SendPacket(const PACKET_HEADER& header) const;
	[[nodiscard]] bool ReceiveByte(char* buffer, uint32_t len) const;
	PACKET_HEADER* ReceivePacket(char* buffer);

	static bool HandlePacket(const PACKET_HEADER& header);

	void ThreadFunc();

	[[nodiscard]] SOCKET GetSocket() const { return socket_; }

   private:
	std::array<char, Config::kClientBufferSize> buffer_{};
	SOCKET socket_;
};
