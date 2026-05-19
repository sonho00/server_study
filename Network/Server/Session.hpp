#pragma once

#include <WinSock2.h>

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
	bool SendPacket(const PACKET_HEADER& header);

	bool HandleIO(OverlappedEx& ovEx, DWORD bytesTransferred);

	bool Connect();
	bool Disconnect();
	bool Clear();

	[[nodiscard]] SOCKET GetSocket() const { return socket_; }
	[[nodiscard]] uint64_t GetHandle() const { return handle_; }
	[[nodiscard]] SessionManager* GetSessionManager() const {
		return sessionManager_;
	}
	[[nodiscard]] Listener* GetListener() const { return listener_; }
	void SetListener(Listener* listener) { listener_ = listener; }

	OverlappedEx readOv_;
	OverlappedEx writeOv_;
	OverlappedEx disconnectOv_;

   private:
	bool RegisterWriteInternal();

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);

	SOCKET socket_ = INVALID_SOCKET;
	uint64_t handle_ = SparseSet<Config::kPoolSize>::kInvalidHandle;

	SessionManager* sessionManager_ = nullptr;
	Listener* listener_ = nullptr;

	bool isSending_ = false;

	std::mutex writeMtx_;
	std::mutex connectMtx_;
};
