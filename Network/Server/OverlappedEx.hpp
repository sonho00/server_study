#pragma once

#include <WinSock2.h>
#include <minwinbase.h>

#include "Network/Common/MagicBuffer.hpp"
#include "Network/Common/Protocol.hpp"
#include "Network/Common/SharedPoolPtr.hpp"

class Session;

struct OverlappedEx {
	OverlappedEx(size_t bufferSize = Config::kMagicBufferSize)
		: buffer_(bufferSize) {}

	void Reset() {
		overlapped_ = {};
		ioType_ = IO_TYPE::kNone;
		wsaBuf_ = {};
		buffer_.Clear();
		sessionPtr_.Reset();
		readPos_ = 0;
		writePos_ = 0;
	}

	OVERLAPPED overlapped_ = {};
	IO_TYPE ioType_ = IO_TYPE::kNone;

	WSABUF wsaBuf_ = {};
	MagicBuffer buffer_;
	SharedPoolPtr<Session> sessionPtr_ = nullptr;

	size_t readPos_ = 0;
	size_t writePos_ = 0;
};
