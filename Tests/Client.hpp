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

	virtual void ThreadFunc() = 0;

	bool test();

	bool success_ = true;
	size_t testBytes_ = 0;

	static std::array<char, 1 << 20> sendBuf_;
	static std::array<char, 1 << 20> recvBuf_;

   protected:
	SOCKET socket_;
};
