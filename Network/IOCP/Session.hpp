#pragma once

#include <WinSock2.h>
#include <minwindef.h>

#include <functional>

#include "ObjectPool.hpp"
#include "OverlappedEx.hpp"

class Session : public PoolElement<Session> {
   public:
	constexpr static size_t kReadBufferSize = 256;
	constexpr static size_t kWriteBufferSize = 256;

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);
	bool HandleIO(OverlappedEx<kReadBufferSize>* ovEx, DWORD bytesTransferred);
	void Close();

	OverlappedEx<kReadBufferSize> readOv;
	OverlappedEx<kWriteBufferSize> writeOv;
	SOCKET socket_ = INVALID_SOCKET;
};
