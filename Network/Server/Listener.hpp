#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <cstdint>

#include "OverlappedEx.hpp"

class IocpCore;
class SessionManager;

class Listener {
   public:
	Listener(IocpCore& iocpCore, SessionManager& sessionManager, uint16_t port,
			 LPFN_ACCEPTEX acceptEx = nullptr);
	~Listener();

	bool HandleAccept(const OverlappedEx& ovEx);
	bool PostAccept();
	bool RegisterAccept(SOCKET hAcceptSocket, SharedPoolPtr<Session> session);

   private:
	OverlappedEx acceptOv_;
	SOCKET socket_ = INVALID_SOCKET;

	IocpCore& iocpCore_;
	SessionManager& sessionManager_;
	uint16_t port_;
	LPFN_ACCEPTEX acceptEx_;
};
