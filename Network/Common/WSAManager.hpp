#pragma once

#include <WinSock2.h>

#include "NetUtils.hpp"

class WSAManager {
   public:
	WSAManager() {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			NetUtils::PrintError("WSAStartup failed");
			exit(1);
		}
	}

	~WSAManager() { WSACleanup(); }
};
