#pragma once

#include <WinSock2.h>

#include "Network/Common/MagicBuffer.hpp"
#include "Network/Common/Protocol.hpp"

class Session;

struct OverlappedEx {
	OverlappedEx(size_t bufferSize = Config::kMagicBufferSize)
		: buffer_(bufferSize) {}

	OVERLAPPED overlapped_ = {};
	IO_TYPE ioType_ = IO_TYPE::kNone;

	WSABUF wsaBuf_ = {};
	MagicBuffer buffer_;

	size_t readPos_ = 0;
	size_t writePos_ = 0;
};
