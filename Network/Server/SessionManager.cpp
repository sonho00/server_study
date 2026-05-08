#include "SessionManager.hpp"

#include <cstdint>

#include "Network/Common/Logger.hpp"
#include "Network/Common/SharedPoolPtr.hpp"
#include "Network/Common/SparsePool.hpp"

SharedPoolPtr<Session> SessionManager::CreateSession() {
	SharedPoolPtr<Session> session = sessionPool_.Acquire();
	if (!session.IsValid()) {
		LOG_ERROR("Failed to create session: No available handles");
		return nullptr;
	}
	session->SetSessionManager(this);
	uint64_t handle = session->GetHandle();
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx] = session;

	return session;
}

bool SessionManager::AddSession(uint64_t handle) {
	if (!activeSessions_.Pop(handle)) {
		LOG_ERROR("Failed to add session with handle: {}", handle);
		return false;
	}
	return true;
}

bool SessionManager::ReleaseSession(uint64_t handle) {
	auto idx = static_cast<uint32_t>(handle);
	activeSessions_.Push(handle);
	sessionPtrs_[idx].Reset();
	return true;
}

SharedPoolPtr<Session> SessionManager::GetSession(uint64_t handle) {
	if (!sessionPool_.IsValid(handle)) return nullptr;

	auto idx = static_cast<uint32_t>(handle);
	return sessionPtrs_[idx];
}
