#pragma once

#include <WinSock2.h>
#include <minwindef.h>

#include <atomic>
#include <functional>
#include <queue>

#include "ObjectPool.hpp"
#include "OverlappedEx.hpp"
#include "Protocol.hpp"


class Session : public PoolElement<Session> {
   public:
	constexpr static size_t kReadBufferSize = 256;
	constexpr static size_t kWriteBufferSize = 256;

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);
	bool HandleIO(OverlappedEx<kReadBufferSize>* ovEx, DWORD bytesTransferred);

	bool HandleC2S_MOVE(C2S_MOVE* packet);
	bool HandleC2S_CHAT(C2S_CHAT* packet);

	bool Broadcast(PACKET* packet);

	void Close();

	OverlappedEx<kReadBufferSize> readOv;
	OverlappedEx<kWriteBufferSize> writeOv;
	SOCKET socket_ = INVALID_SOCKET;
	std::queue<std::string> sendQueue_;
	std::atomic<bool> isSending_ = false;

	std::function<bool()> Handlers[static_cast<size_t>(C2S_PACKET_ID::CNT)] = {
		[]() { return false; },
		[this]() {
			return HandleC2S_MOVE(
				reinterpret_cast<C2S_MOVE*>(readOv.buffer_ + readOv.readPos_));
		},
		[this]() {
			return HandleC2S_CHAT(
				reinterpret_cast<C2S_CHAT*>(readOv.buffer_ + readOv.readPos_));
		}};
};
