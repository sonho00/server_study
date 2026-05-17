#pragma once

#include <array>
#include <cstdint>

#include "Network/Common/Config.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "Network/Common/Pool/SparsePool.hpp"
#include "Network/Common/Protocol.hpp"
#include "Session.hpp"

class IocpCore;
class Listener;

class SessionManager {
   public:
	SessionManager();

	bool Init(IocpCore& iocpCore, Listener& listener);
	bool RegisterSession(uint64_t handle);

	SharedPoolPtr<Session> CreateSession();
	bool ConnectSession(uint64_t handle);
	void DisconnectSession(uint64_t handle);

	bool Broadcast(const PACKET_HEADER& header, uint64_t sessionHandle);

	SharedPoolPtr<Session> GetSession(uint64_t handle);
	SessionState GetState(uint64_t handle);
	bool SetState(uint64_t handle, SessionState newState);

   private:
	IocpCore* iocpCore_ = nullptr;
	SparsePool<Session, Config::kPoolSize,
			   static_cast<size_t>(SessionState::kCnt)>
		sessionPool_;
	std::array<SharedPoolPtr<Session>, Config::kPoolSize> sessionPtrs_;
};
