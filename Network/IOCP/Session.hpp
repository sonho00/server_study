#pragma once

#include <WinSock2.h>

enum class Task { NONE, ACCEPT, READ, WRITE };

class Session {
   public:
	bool OnRead(DWORD bytesTransferred);
	bool OnWrite();

	OVERLAPPED overlapped_ = {};
	SOCKET socket_ = INVALID_SOCKET;
	WSABUF wsaBuf_ = {};
	char buffer_[1024]{};
	Task taskType_ = Task::NONE;
};
