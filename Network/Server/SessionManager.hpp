#pragma once

#include <array>

#include "Network/Common/Config.hpp"
#include "Network/Common/SharedPoolPtr.hpp"
#include "Network/Common/SparsePool.hpp"
#include "Network/Common/SparseSet.hpp"
#include "Session.hpp"

class SessionManager {
   public:
	SharedPoolPtr<Session> CreateSession();
	bool AddSession(uint64_t handle);
	bool RemoveSession(uint64_t handle);
	SharedPoolPtr<Session> GetSession(uint64_t handle);

   private:
	SparsePool<Session, Config::kPoolSize> sessionPool_;
	SparseSet<Config::kPoolSize> activeSessions_;
	std::array<SharedPoolPtr<Session>, Config::kPoolSize> sessionPtrs_;
};
