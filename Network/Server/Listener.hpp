#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <cstdint>

#include "OverlappedEx.hpp"

class IocpCore;

class Listener {
   public:
	Listener(IocpCore& iocpCore, const uint16_t port,
			 const LPFN_ACCEPTEX acceptEx = nullptr);
	~Listener();

	bool HandleAccept(const OverlappedEx& ovEx);
	bool PostAccept();

   private:
	OverlappedEx acceptOv;
	SOCKET socket_ = INVALID_SOCKET;

	IocpCore& iocpCore_;
	const uint16_t port_;
	LPFN_ACCEPTEX acceptEx_;
};
