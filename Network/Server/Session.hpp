#pragma once

#include <WinSock2.h>

#include <array>
#include <cstddef>
#include <functional>
#include <mutex>

#include "Network/Common/Pool/SparseSet.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"

enum class SessionState : uint8_t {
	kIdle,
	kPending,
	kConnected,
	kDisconnecting,
	kCnt
};

class SessionManager;
class Listener;

class Session {
	friend class SessionManager;

   public:
	Session();

	bool RegisterRead();
	bool RegisterWrite();
	bool SendPacket(const PACKET_HEADER& header);

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);
	bool HandleIO(OverlappedEx& ovEx, DWORD bytesTransferred);

	bool Disconnect();
	bool Clear();

	[[nodiscard]] uint64_t GetHandle() const { return handle_; }
	[[nodiscard]] SOCKET GetSocket() const { return socket_; }

	OverlappedEx readOv_;
	OverlappedEx writeOv_;
	OverlappedEx disconnectOv_;
	Listener* listener_ = nullptr;

   private:
	SessionManager* sessionManager_ = nullptr;
	SOCKET socket_ = INVALID_SOCKET;
	uint64_t handle_ = SparseSet<Config::kPoolSize>::kInvalidHandle;
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
