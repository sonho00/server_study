#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include "OverlappedEx.hpp"

class IocpCore;

class Listener {
   public:
	Listener(IocpCore* iocpCore, unsigned short port, LPFN_ACCEPTEX acceptEx);
	~Listener();

	bool HandleAccept(OverlappedEx* overlappedEx, DWORD bytesTransferred);
	bool PostAccept();

   private:
	OverlappedEx acceptOv;
	SOCKET socket_ = INVALID_SOCKET;

	IocpCore* iocpCore_ = nullptr;
	unsigned short port_ = 0;
	LPFN_ACCEPTEX acceptEx_ = nullptr;
};
