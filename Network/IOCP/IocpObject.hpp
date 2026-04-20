#pragma once

#include <WinSock2.h>

enum class Task { NONE, ACCEPT, READ, WRITE };

class IocpObject {
   public:
	virtual ~IocpObject() {
		if (socket_ != INVALID_SOCKET) {
			closesocket(socket_);
		}
	}
	virtual bool Dispatch(OVERLAPPED* overlapped, DWORD bytesTransferred) = 0;

	OVERLAPPED overlapped_ = {};
	SOCKET socket_ = INVALID_SOCKET;
	Task taskType_ = Task::NONE;
	WSABUF wsaBuf_ = {};
};
