#pragma once

#include <WinSock2.h>

#include "IocpCore.hpp"
#include "OverlappedEx.hpp"

class Listener {
   public:
	Listener(IocpCore* iocpCore, unsigned short port)
		: iocpCore_(iocpCore), port_(port) {}

	bool Init();
	bool HandleAccept(OverlappedEx* overlappedEx, DWORD bytesTransferred);
	bool PostAccept();

   private:
	OverlappedEx acceptOv;
	SOCKET socket_ = INVALID_SOCKET;

	IocpCore* iocpCore_ = nullptr;
	unsigned short port_ = 0;
};
