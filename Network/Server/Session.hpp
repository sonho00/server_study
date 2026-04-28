#pragma once

#include <WinSock2.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>

#include "Network/Common/ObjectPool.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"

class SessionManager;

class Session : public PoolElement<Session> {
   public:
	Session(const size_t readBufferSize = 1 << 16,
			const size_t writeBufferSize = 1 << 16)
		: readOv(readBufferSize), writeOv(writeBufferSize) {}

	bool RegisterRead();
	bool RegisterWrite();
	bool SendPacket(const PACKET_HEADER& packet);

	bool OnRead(const DWORD bytesTransferred);
	bool OnWrite(const DWORD bytesTransferred);
	bool HandleIO(OverlappedEx& ovEx, const DWORD bytesTransferred);

	void Close();

	size_t GetSessionId() const { return sessionId_; }
	void SetSessionId(const size_t sessionId) { sessionId_ = sessionId; }

	OverlappedEx readOv = {};
	OverlappedEx writeOv = {};
	SOCKET socket_ = INVALID_SOCKET;

   private:
	size_t sessionId_ = 0;
	std::mutex mtx;
	bool isSending_ = false;

	std::function<bool(const DWORD bytesTransferred)>
		inputHandlers[static_cast<size_t>(IO_TYPE::CNT)] = {
			nullptr,
			[this](const DWORD bytesTransferred) {
				return OnRead(bytesTransferred);
			},
			[this](const DWORD bytesTransferred) {
				return OnWrite(bytesTransferred);
			}};
};
