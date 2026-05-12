#include "SessionManager.hpp"

#include <cstdint>

#include "Listener.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "Network/Common/Pool/SparsePool.hpp"
#include "Session.hpp"

SessionManager::SessionManager()
	: sessionPool_([this](Session* session) {
		  LOG_DEBUG("[Session:{}] Custom deleter called", session->handle_);
		  LOG_DEBUG("this:{}", static_cast<void*>(this));
		  session->handle_ = SparseSet<Config::kPoolSize>::kInvalidHandle;
		  session->listener_->PostAccept();
	  }) {}

SharedPoolPtr<Session> SessionManager::CreateSession() {
	SharedPoolPtr<Session> sessionPtr =
		sessionPool_.Acquire(static_cast<size_t>(SessionState::kIdle));
	if (!sessionPtr.IsValid()) {
		LOG_WARN("Failed to create session: No available handles");
		return nullptr;
	}

	uint64_t handle = sessionPtr.GetHandle();
	sessionPool_.MoveToState(handle,
							 static_cast<size_t>(SessionState::kPending));
	sessionPtr->sessionManager_ = this;
	sessionPtr->handle_ = handle;
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx] = std::move(sessionPtr);
	return sessionPtrs_[idx];
}

bool SessionManager::ConnectSession(uint64_t handle) {
	return sessionPool_.MoveToState(
		handle, static_cast<size_t>(SessionState::kConnected));
}

bool SessionManager::DisconnectSession(uint64_t handle) {
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx].Reset();
	return SetState(handle, SessionState::kDisconnecting);
}

SharedPoolPtr<Session> SessionManager::GetSession(uint64_t handle) {
	if (!sessionPool_.IsValid(handle)) {
		LOG_ERROR("Invalid session handle: {}", handle);
		return nullptr;
	}

	auto idx = static_cast<uint32_t>(handle);
	return sessionPtrs_[idx];
}

bool SessionManager::SetState(uint64_t handle, SessionState newState) {
	return sessionPool_.MoveToState(handle, static_cast<size_t>(newState));
}
