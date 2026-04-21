#pragma once

#include <WinSock2.h>

enum class Task { NONE, ACCEPT, RECV, SEND };

template <size_t BufferSize>
struct OverlappedEx {
	OVERLAPPED overlapped_ = {};
	Task taskType_ = Task::NONE;
	WSABUF wsaBuf_ = {};
	char buffer_[BufferSize] = {};

	void Reset() {
		overlapped_ = {};
		taskType_ = Task::NONE;
		wsaBuf_ = {};
	}
};
