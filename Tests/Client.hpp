#pragma once

#include <WinSock2.h>

#include "Network/Common/Protocol.hpp"

SOCKET CreateClientSocket(const char* ip, uint16_t port);

class Client {
   public:
	Client(const char* ip, const uint16_t port);
	~Client();

	SOCKET GetSocket() const { return socket_; }

	bool SendPacket(const PACKET_HEADER* header);
	bool ReceiveByte(char* buffer, const size_t bufferSize);

	bool HandlePacket(const PACKET_HEADER* header);

	virtual void ThreadFunc() = 0;

   protected:
	char buffer_[1024];
	const SOCKET socket_;
};
