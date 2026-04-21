#pragma once

#include <WinSock2.h>

#include "IocpCore.hpp"
#include "OverlappedEx.hpp"

class Listener {
   public:
	Listener(IocpCore* iocpCore, unsigned short port)
		: iocpCore_(iocpCore), port_(port) {}

	bool Init();
	bool HandleAccept(OverlappedEx<Session::kReadBufferSize>* overlappedEx,
					  DWORD bytesTransferred);
	bool PostAccept();

   private:
	OverlappedEx<sizeof(sockaddr_in) + 16> acceptOv;
	SOCKET socket_ = INVALID_SOCKET;

	IocpCore* iocpCore_ = nullptr;
	unsigned short port_ = 0;
};
