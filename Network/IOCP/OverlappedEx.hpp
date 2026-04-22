#pragma once

#include <WinSock2.h>

#include <cstddef>

enum class Task { NONE, ACCEPT, RECV, SEND };

template <size_t BufferSize>
struct OverlappedEx {
	OVERLAPPED overlapped_ = {};
	Task taskType_ = Task::NONE;
	WSABUF wsaBuf_ = {};
	char buffer_[BufferSize] = {};
	size_t readPos_ = 0;
	size_t writePos_ = 0;
};
