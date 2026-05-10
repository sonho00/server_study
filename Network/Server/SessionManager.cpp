#include "SessionManager.hpp"

#include <cstdint>

#include "Network/Common/Logger.hpp"
#include "Network/Common/SharedPoolPtr.hpp"
#include "Network/Common/SparsePool.hpp"

SharedPoolPtr<Session> SessionManager::CreateSession() {
	SharedPoolPtr<Session> sessionPtr =
		sessionPool_.Acquire(static_cast<size_t>(SessionState::kPending));
	if (!sessionPtr.IsValid()) {
		LOG_ERROR("Failed to create session: No available handles");
		return nullptr;
	}
	sessionPtr->sessionManager_ = this;
	sessionPtr->handle_ = sessionPtr.GetHandle();
	auto idx = static_cast<uint32_t>(sessionPtr->handle_);
	sessionPtrs_[idx] = std::move(sessionPtr);
	return sessionPtrs_[idx];
}

bool SessionManager::ConnectSession(uint64_t handle) {
	return sessionPool_.MoveToState(handle,
									static_cast<size_t>(SessionState::kActive));
}

bool SessionManager::DisconnectSession(uint64_t handle) {
	return sessionPool_.Release(handle);
}

SharedPoolPtr<Session> SessionManager::GetSession(uint64_t handle) {
	if (!sessionPool_.IsValid(handle)) return nullptr;

	auto idx = static_cast<uint32_t>(handle);
	return sessionPtrs_[idx];
}
