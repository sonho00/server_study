#pragma once

#include <WinSock2.h>

#include <atomic>

#include "Network/Common/MagicBuffer.hpp"
#include "Network/Common/Protocol.hpp"

struct OverlappedEx {
	OverlappedEx(const size_t bufferSize = 1 << 16) : buffer_(bufferSize) {}

	OVERLAPPED overlapped_ = {};
	IO_TYPE ioType_ = IO_TYPE::NONE;

	WSABUF wsaBuf_ = {};
	MagicBuffer buffer_;

	std::atomic<size_t> readPos_ = 0;
	std::atomic<size_t> writePos_ = 0;
};
