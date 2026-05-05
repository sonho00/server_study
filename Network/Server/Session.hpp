#pragma once

#include <WinSock2.h>

#include <array>
#include <cstddef>
#include <functional>
#include <mutex>

#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"

class SessionManager;

class Session {
   public:
	Session()
		: readOv_(Config::kMagicBufferSize),
		  writeOv_(Config::kMagicBufferSize) {}

	bool RegisterRead();
	bool RegisterWrite();
	bool SendPacket(const PACKET_HEADER& header);

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);
	bool HandleIO(OverlappedEx& ovEx, DWORD bytesTransferred);

	void Close();
	void Reset();

	[[nodiscard]] uint64_t GetHandle() const { return handle_; }
	void SetHandle(uint64_t handle) { handle_ = handle; }

	void SetSessionManager(SessionManager* manager) {
		sessionManager_ = manager;
	}

	OverlappedEx readOv_;
	OverlappedEx writeOv_;
	SOCKET socket_ = INVALID_SOCKET;

   private:
	SessionManager* sessionManager_ = nullptr;
	uint64_t handle_ = 0;
	std::mutex mtx_;
	bool isSending_ = false;

	std::array<std::function<bool(DWORD bytesTransferred)>,
			   static_cast<size_t>(IO_TYPE::kCnt)>
		inputHandlers_ = {
			nullptr,
			[this](DWORD bytesTransferred) { return OnRead(bytesTransferred); },
			[this](DWORD bytesTransferred) {
				return OnWrite(bytesTransferred);
			}};
};
