#pragma once

#include <WinSock2.h>
#include <minwindef.h>
#include <rpcndr.h>

#include <atomic>
#include <functional>

#include "ObjectPool.hpp"
#include "OverlappedEx.hpp"
#include "Protocol.hpp"

class Session : public PoolElement<Session> {
   public:
	Session(size_t readBufferSize = 1 << 16, size_t writeBufferSize = 1 << 16)
		: readOv(readBufferSize), writeOv(writeBufferSize) {}

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);
	bool HandleIO(OverlappedEx* ovEx, DWORD bytesTransferred);

	bool HandleC2S_MOVE(C2S_MOVE* packet);
	bool HandleC2S_CHAT(C2S_CHAT* packet);

	bool Broadcast(PACKET* packet);

	void Close();

	OverlappedEx readOv = {};
	OverlappedEx writeOv = {};
	SOCKET socket_ = INVALID_SOCKET;

	std::atomic<bool> isSending_ = false;

	std::function<bool(DWORD bytesTransferred)>
		inputHandlers[static_cast<size_t>(Task::CNT)] = {
			nullptr,
			[this](DWORD bytesTransferred) { return OnRead(bytesTransferred); },
			[this](DWORD bytesTransferred) {
				return OnWrite(bytesTransferred);
			}};

   private:
	std::function<bool()>
		packetHandlers[static_cast<size_t>(C2S_PACKET_ID::CNT)] = {
			nullptr,
			[this]() {
				return HandleC2S_MOVE(reinterpret_cast<C2S_MOVE*>(
					readOv.buffer_.GetBuffer() + readOv.readPos_));
			},
			[this]() {
				return HandleC2S_CHAT(reinterpret_cast<C2S_CHAT*>(
					readOv.buffer_.GetBuffer() + readOv.readPos_));
			}};
};
