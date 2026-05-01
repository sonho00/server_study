#pragma once

#include <memory>

#include "Network/Common/ObjectPool.hpp"
#include "Network/Common/SparseSet.hpp"
#include "Session.hpp"

class SessionManager {
   public:
	std::shared_ptr<Session> CreateSession();
	bool AddSession(Session& session);
	bool RemoveSession(uint64_t sessionHandle);

   private:
	SparseSet<Config::kPoolSize> createdSessions_;
	SparseSet<Config::kPoolSize> activeSessions_;
	ObjectPool<Session, Config::kPoolSize> sessionPool_;
	std::array<std::shared_ptr<Session>, Config::kPoolSize> sessionHandles_;
};
