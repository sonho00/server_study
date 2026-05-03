#include "SessionManager.hpp"

#include <cstdint>

#include "Network/Common/Logger.hpp"
#include "Network/Common/SharedPoolPtr.hpp"
#include "Network/Common/SparsePool.hpp"

SharedPoolPtr<Session> SessionManager::CreateSession() {
	auto session = sessionPool_.Acquire();
	if (!session) {
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
	sessionPtrs_[handle].Reset();
	return true;
}

SharedPoolPtr<Session> SessionManager::GetSession(uint64_t handle) {
	if (!sessionPool_.IsValid(handle)) {
		LOG_DEBUG("Invalid session handle: {}", handle);
		return nullptr;
	}
	auto idx = static_cast<uint32_t>(handle);
	return sessionPtrs_[idx];
}
