#pragma once

#include <WinSock2.h>

#include "ObjectPool.hpp"

enum class Task { NONE, ACCEPT, READ, WRITE };

class IocpObject : public PoolElement<IocpObject> {
   public:
	virtual void Close() {
		Release();
		if (socket_ != INVALID_SOCKET) {
			closesocket(socket_);
			socket_ = INVALID_SOCKET;
		}
	}
	virtual bool Dispatch(OVERLAPPED* overlapped, DWORD bytesTransferred) = 0;

	OVERLAPPED overlapped_ = {};
	SOCKET socket_ = INVALID_SOCKET;
	Task taskType_ = Task::NONE;
	WSABUF wsaBuf_ = {};
};
