#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <atomic>
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
	bool RegisterAccept(SharedPoolPtr<Session>& session);

	SOCKET socket_ = INVALID_SOCKET;
	OverlappedEx acceptOv_;

   private:
	IocpCore& iocpCore_;
	SessionManager& sessionManager_;
	uint16_t port_;

	std::atomic<size_t> pendingAccepts_ = 0;
};
