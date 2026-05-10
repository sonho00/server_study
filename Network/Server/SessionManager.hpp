#pragma once

#include <array>
#include <cstdint>

#include "Network/Common/Config.hpp"
#include "Network/Common/SharedPoolPtr.hpp"
#include "Network/Common/SparsePool.hpp"
#include "Session.hpp"

enum class SessionState : uint8_t { kFree = 0, kPending = 1, kActive = 2 };

class SessionManager {
   public:
	SharedPoolPtr<Session> CreateSession();
	bool ConnectSession(uint64_t handle);
	bool DisconnectSession(uint64_t handle);
	SharedPoolPtr<Session> GetSession(uint64_t handle);

   private:
	SparsePool<Session, Config::kPoolSize, 3> sessionPool_;
	std::array<SharedPoolPtr<Session>, Config::kPoolSize> sessionPtrs_;
};
