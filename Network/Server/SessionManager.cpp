#include "SessionManager.hpp"

#include "Network/Common/Logger.hpp"
#include "Network/Common/SparseSet.hpp"

std::shared_ptr<Session> SessionManager::CreateSession() {
	auto session = createdSessions_.Acquire();
	if (session == nullptr) {
		LOG_ERROR("Failed to create session: No available handles");
		return nullptr;
	}

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
	if (!createdSessions_.Release(sessionHandle)) {
		LOG_ERROR("Failed to return session handle to createdSessions: {}",
				  sessionHandle);
		return false;
	}
	sessionHandles_[sessionHandle].reset();
	return true;
}
