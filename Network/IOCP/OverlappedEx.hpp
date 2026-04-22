#pragma once

#include <WinSock2.h>

#include "MagicBuffer.hpp"
#include "Network/Protocol/Protocol.hpp"

struct OverlappedEx {
	OverlappedEx(size_t bufferSize = 1 << 16)
		: buffer_(bufferSize), ptr_(buffer_.GetBuffer()) {}

	OVERLAPPED overlapped_ = {};
	IO_TYPE ioType_ = IO_TYPE::NONE;

	WSABUF wsaBuf_ = {};
	MagicBuffer buffer_;
	char* ptr_ = nullptr;

	size_t readPos_ = 0;
	size_t writePos_ = 0;
};
