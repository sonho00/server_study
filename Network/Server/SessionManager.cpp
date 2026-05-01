#include "SessionManager.hpp"

#include "Network/Common/Logger.hpp"

std::shared_ptr<Session> SessionManager::CreateSession() {
	auto handle = createdSessions_.Pop();
	if (!handle.has_value()) {
		LOG_ERROR("Failed to create session: No available handles");
		return nullptr;
	}
	auto session = sessionPool_.Acquire(handle.value());
	session->SetHandle(handle.value());
	return session;
}

bool SessionManager::AddSession(Session& session) {
	uint64_t sessionHandle = session.GetHandle();
	if (!activeSessions_.Pop(sessionHandle)) {
		LOG_ERROR("Failed to add session with handle: {}", sessionHandle);
		return false;
	}
	sessionHandles_[sessionHandle] = session.shared_from_this();
	return true;
}

bool SessionManager::RemoveSession(uint64_t sessionHandle) {
	if (!activeSessions_.Push(sessionHandle)) {
		LOG_ERROR("Failed to remove session with handle: {}", sessionHandle);
		return false;
	}
	if (!createdSessions_.Push(sessionHandle)) {
		LOG_ERROR("Failed to return session handle to createdSessions: {}",
				  sessionHandle);
		return false;
	}
	sessionHandles_[sessionHandle].reset();
	return true;
}
