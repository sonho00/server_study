#pragma once

#include <WinSock2.h>

#include <atomic>
#include <functional>

#include "Network/Common/ObjectPool.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"

class Session : public PoolElement<Session> {
   public:
	Session(const size_t readBufferSize = 1 << 16,
			const size_t writeBufferSize = 1 << 16)
		: readOv(readBufferSize), writeOv(writeBufferSize) {}

	bool RegisterRead();
	bool RegisterWrite(const char* packet, const size_t packetSize);

	bool OnRead(const DWORD bytesTransferred);
	bool OnWrite(const DWORD bytesTransferred);
	bool HandleIO(OverlappedEx* ovEx, const DWORD bytesTransferred);

	void Close();

	OverlappedEx readOv = {};
	OverlappedEx writeOv = {};
	SOCKET socket_ = INVALID_SOCKET;

   private:
	std::atomic<bool> isSending_ = false;

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
