#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <cstdint>

#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "OverlappedEx.hpp"

class IocpCore;
class SessionManager;

class Listener {
   public:
	Listener(IocpCore& iocpCore, SessionManager& sessionManager, uint16_t port);
	~Listener();

	bool HandleAccept(SharedPoolPtr<Session>& session);
	bool PostAccept();
	bool RegisterAccept(SOCKET hAcceptSocket, SharedPoolPtr<Session>& session);

	OverlappedEx acceptOv_;

   private:
	SOCKET socket_ = INVALID_SOCKET;

	IocpCore& iocpCore_;
	SessionManager& sessionManager_;
	uint16_t port_;
};
