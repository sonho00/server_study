#include "SessionManager.hpp"

std::shared_ptr<Session> SessionManager::CreateSession() {
	auto session = sessionPool_.Acquire();
	if (!session) {
		return nullptr;
	}

	session->SetSessionManager(shared_from_this());
	return session;
}

void SessionManager::AddSession(std::shared_ptr<Session> session) {
	session->SetSessionId(nextSessionId_++);

	std::lock_guard<std::mutex> lock(mutex_);
	sessions_[session->GetSessionId()] = session;
}

void SessionManager::RemoveSession(size_t sessionId) {
	std::lock_guard<std::mutex> lock(mutex_);
	sessions_.erase(sessionId);
}
