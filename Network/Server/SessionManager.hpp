#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "Network/Common/ObjectPool.hpp"
#include "Session.hpp"

class SessionManager {
   public:
	std::shared_ptr<Session> CreateSession();
	void AddSession(Session& session);
	void RemoveSession(size_t sessionId);

   private:
	ObjectPool<Session, Config::kPoolSize> sessionPool_;
	std::unordered_map<size_t, std::shared_ptr<Session>> sessions_;
	std::mutex mutex_;
	std::atomic<size_t> nextSessionId_ = 1;
};
