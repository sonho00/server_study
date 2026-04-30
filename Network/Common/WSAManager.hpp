#pragma once

#include <WinSock2.h>

#include "Logger.hpp"

class WSAManager {
   public:
	WSAManager() {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			LOG_FATAL("WSAStartup failed: {}", WSAGetLastError());
		}
	}

	~WSAManager() { WSACleanup(); }
};
