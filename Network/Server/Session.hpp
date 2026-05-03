#pragma once

#include <WinSock2.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>

#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"

class SessionManager;

struct BufferSizes {
	size_t readBufferSize_ = 1 << 16;
	size_t writeBufferSize_ = 1 << 16;
};

class Session : public std::enable_shared_from_this<Session> {
   public:
	Session(const BufferSizes& bufferSizes = BufferSizes())
		: readOv_(bufferSizes.readBufferSize_),
		  writeOv_(bufferSizes.writeBufferSize_) {}

	bool RegisterRead();
	bool RegisterWrite();
	bool SendPacket(const PACKET_HEADER& header);

	bool OnRead(DWORD bytesTransferred);
	bool OnWrite(DWORD bytesTransferred);
	bool HandleIO(OverlappedEx& ovEx, DWORD bytesTransferred);

	void Close();

	[[nodiscard]] uint64_t GetHandle() const { return handle_; }
	void SetHandle(uint64_t handle) { handle_ = handle; }

	OverlappedEx readOv_;
	OverlappedEx writeOv_;
	SOCKET socket_ = INVALID_SOCKET;

   private:
	uint64_t handle_ = 0;
	std::mutex mtx_;
	bool isSending_ = false;

	std::array<std::function<bool(DWORD bytesTransferred)>,
			   static_cast<size_t>(IO_TYPE::kCnt)>
		inputHandlers_ = {
			nullptr,
			[this](DWORD bytesTransferred) { return OnRead(bytesTransferred); },
			[this](DWORD bytesTransferred) {
				return OnWrite(bytesTransferred);
			}};
};
