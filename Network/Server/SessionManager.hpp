#pragma once

#include <array>
#include <cstdint>

#include "Network/Common/Config.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "Network/Common/Pool/SparsePool.hpp"
#include "Session.hpp"

class IocpCore;
class Listener;

class SessionManager {
   public:
	SessionManager();

	bool Init(IocpCore& iocpCore);

	SharedPoolPtr<Session> CreateSession();
	bool ConnectSession(uint64_t handle);
	void DisconnectSession(uint64_t handle);

	SharedPoolPtr<Session> GetSession(uint64_t handle);
	SessionState GetState(uint64_t handle);
	bool SetState(uint64_t handle, SessionState newState);

   private:
	SparsePool<Session, Config::kPoolSize,
			   static_cast<size_t>(SessionState::kCnt)>
		sessionPool_;
	std::array<SharedPoolPtr<Session>, Config::kPoolSize> sessionPtrs_;
};
