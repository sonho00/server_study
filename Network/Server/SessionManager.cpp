#include "SessionManager.hpp"

std::shared_ptr<Session> SessionManager::CreateSession() {
	auto session = sessionPool_.Acquire();
	if (!session) {
		return nullptr;
	}

	return session;
}

void SessionManager::AddSession(Session& session) {
	session.SetSessionId(nextSessionId_++);

	std::lock_guard<std::mutex> lock(mutex_);
	sessions_[session.GetSessionId()] = session.shared_from_this();
}

void SessionManager::RemoveSession(size_t sessionId) {
	std::lock_guard<std::mutex> lock(mutex_);
	sessions_.erase(sessionId);
}
