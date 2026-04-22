#pragma once

#include <WinSock2.h>

#include <atomic>
#include <functional>

#include "Network/Protocol/Protocol.hpp"
#include "ObjectPool.hpp"
#include "OverlappedEx.hpp"

class Session : public PoolElement<Session> {
   public:
	Session(size_t readBufferSize = 1 << 16, size_t writeBufferSize = 1 << 16)
		: readOv(readBufferSize), writeOv(writeBufferSize) {}

	bool RegisterRead();
	bool RegisterWrite(void* packet, size_t packetSize);

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);
	bool HandleIO(OverlappedEx* ovEx, DWORD bytesTransferred);

	bool Broadcast(void* packet);

	void Close();

	OverlappedEx readOv = {};
	OverlappedEx writeOv = {};
	SOCKET socket_ = INVALID_SOCKET;

	std::atomic<bool> isSending_ = false;

	std::function<bool(DWORD bytesTransferred)>
		inputHandlers[static_cast<size_t>(IO_TYPE::CNT)] = {
			nullptr,
			[this](DWORD bytesTransferred) { return OnRead(bytesTransferred); },
			[this](DWORD bytesTransferred) {
				return OnWrite(bytesTransferred);
			}};
};
