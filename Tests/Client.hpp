#pragma once

#include <WinSock2.h>

#include <array>

#include "Network/Common/Protocol.hpp"

class Client {
   public:
	Client();
	~Client();

	[[nodiscard]] bool SendPacket(const PACKET_HEADER& header) const;
	[[nodiscard]] bool ReceiveByte(char* buffer, uint32_t len) const;
	PACKET_HEADER* ReceivePacket(char* buffer);

	static bool HandlePacket(const PACKET_HEADER& header);

	virtual void ThreadFunc() = 0;
	virtual bool test() = 0;

	bool success_ = true;
	size_t testBytes_ = 0;

	static std::array<char, 1 << 20> sendBuf_;
	static std::array<char, 1 << 20> recvBuf_;

	static constexpr const char* kIpAddr_ = "127.0.0.1";
	static constexpr uint16_t kPort_ = 12345;

   protected:
	SOCKET socket_;
};
