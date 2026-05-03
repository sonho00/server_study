#include "SessionManager.hpp"

#include <cstdint>

#include "Network/Common/Logger.hpp"
#include "Network/Common/SparseSet.hpp"

std::shared_ptr<Session> SessionManager::CreateSession() {
	auto session = sessionPool_.Acquire();
	if (session == nullptr) {
		LOG_ERROR("Failed to create session: No available handles");
		return nullptr;
	}
	uint64_t handle = session->GetHandle();
	sessionPtrs_[handle] = session;

	return session;
}

bool SessionManager::AddSession(uint64_t handle) {
	if (!activeSessions_.Pop(handle)) {
		LOG_ERROR("Failed to add session with handle: {}", handle);
		return false;
	}
	return true;
}

bool SessionManager::RemoveSession(uint64_t handle) {
	if (!activeSessions_.Push(handle)) {
		LOG_ERROR("Failed to remove session with handle: {}", handle);
		return false;
	}
	if (!sessionPool_.Release(handle)) {
		LOG_ERROR("Failed to return session handle to sessionPool: {}", handle);
		return false;
	}
	sessionPtrs_[handle].reset();
	return true;
}
