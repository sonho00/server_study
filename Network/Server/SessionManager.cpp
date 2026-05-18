#include "SessionManager.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "IocpCore.hpp"
#include "Listener.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "Network/Common/Pool/SparsePool.hpp"
#include "Session.hpp"

SessionManager::SessionManager()
	: sessionPool_([](Session* sessionPtr) {
		  sessionPtr->readOv_.Reset();
		  sessionPtr->writeOv_.Reset();
		  sessionPtr->handle_ = ISparsePool<Session>::kInvalidHandle;
	  }) {}

bool SessionManager::Init(IocpCore& iocpCore, Listener& listener) {
	iocpCore_ = &iocpCore;
	sessionPool_.SetPostReleaseFunc([&listener] { listener.PostAccept(); });

	std::vector<uint64_t> handles = sessionPool_.GetIndicesInState(
		static_cast<size_t>(SessionState::kIdle));

	return std::ranges::all_of(handles, [this](uint64_t handle) {
		Session* sessionPtr = sessionPool_.GetObj(handle);
		if (!iocpCore_->Register(sessionPtr->socket_, handle)) {
			LOG_ERROR("Failed to register accept socket with IOCP");
			return false;
		}
		return true;
	});
}

bool SessionManager::RegisterSession(uint64_t handle) {
	Session* sessionPtr = sessionPool_.GetObj(handle);
	if (!iocpCore_->Register(sessionPtr->socket_, handle)) {
		LOG_ERROR("Failed to register accept socket with IOCP");
		return false;
	}
	return true;
}

SharedPoolPtr<Session> SessionManager::CreateSession() {
	SharedPoolPtr<Session> sessionPtr =
		sessionPool_.Acquire(static_cast<size_t>(SessionState::kPending));
	if (!sessionPtr.IsValid()) {
		LOG_WARN("Failed to acquire session from pool");
		return nullptr;
	}

	uint64_t handle = sessionPtr.GetHandle();
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

void SessionManager::DisconnectSession(uint64_t handle) {
	if (!sessionPool_.IsValid(handle)) {
		LOG_ERROR("Invalid session handle: {}", handle);
		return;
	}
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx].Reset();
}

bool SessionManager::Broadcast(const PACKET_HEADER& header,
							   uint64_t sessionHandle) {
	std::vector<uint64_t> handles = sessionPool_.GetIndicesInState(
		static_cast<size_t>(SessionState::kConnected));

	for (uint64_t handle : handles) {
		if (handle == sessionHandle) {
			continue;
		}
		auto idx = static_cast<uint32_t>(handle);
		if (sessionPtrs_[idx].IsValid()) {
			sessionPtrs_[idx]->SendPacket(header);
		}
	}
	return true;
}

SharedPoolPtr<Session> SessionManager::GetSession(uint64_t handle) {
	if (!sessionPool_.IsValid(handle)) {
		LOG_ERROR("Invalid session handle: {}", handle);
		return nullptr;
	}

	auto idx = static_cast<uint32_t>(handle);
	return sessionPtrs_[idx];
}

SessionState SessionManager::GetState(uint64_t handle) {
	return static_cast<SessionState>(sessionPool_.GetState(handle));
}

bool SessionManager::SetState(uint64_t handle, SessionState newState) {
	return sessionPool_.MoveToState(handle, static_cast<size_t>(newState));
}
