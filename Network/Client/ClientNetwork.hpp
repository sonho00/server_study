#pragma once

#include <WinSock2.h>
#include <winsock2.h>

#include "Network/Common/Protocol.hpp"

SOCKET CreateClientSocket(const char* ip, uint16_t port);

struct Client {
	Client(const char* ip, uint16_t port);
	~Client();
	SOCKET socket_;
};

bool SendPacket(SOCKET clientSocket, const PACKET_HEADER* header);
bool ReceivePacket(SOCKET clientSocket, char* buffer, size_t bufferSize);

bool HandlePacket(PACKET_HEADER* header);
