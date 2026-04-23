#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <cstdint>

#include "OverlappedEx.hpp"

class IocpCore;

class Listener {
   public:
	Listener(IocpCore* iocpCore, const uint16_t port,
			 const LPFN_ACCEPTEX acceptEx);
	~Listener();

	bool HandleAccept(const OverlappedEx* overlappedEx);
	bool PostAccept();

   private:
	OverlappedEx acceptOv;
	SOCKET socket_ = INVALID_SOCKET;

	IocpCore* iocpCore_ = nullptr;
	const uint16_t port_ = 0;
	LPFN_ACCEPTEX acceptEx_ = nullptr;
};
