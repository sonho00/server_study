#pragma once

#include <array>
#include <memory>

#include "Network/Common/Config.hpp"
#include "Network/Common/SparsePool.hpp"
#include "Network/Common/SparseSet.hpp"
#include "Session.hpp"

class SessionManager {
   public:
	std::shared_ptr<Session> CreateSession();
	bool AddSession(uint64_t handle);
	bool RemoveSession(uint64_t handle);

   private:
	SparsePool<Session, Config::kPoolSize> sessionPool_;
	SparseSet<Config::kPoolSize> activeSessions_;
	std::array<std::shared_ptr<Session>, Config::kPoolSize> sessionPtrs_;
};
