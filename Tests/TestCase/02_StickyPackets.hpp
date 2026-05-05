#pragma once

#include <cstdio>
#include <cstring>
#include <thread>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class StickyPackets : public Client {
   public:
	void ThreadFunc() override {
		reinterpret_cast<PACKET_HEADER*>(sendBuf_.data())->id =
			static_cast<uint16_t>(C2S_PACKET_ID::kChat);
		reinterpret_cast<PACKET_HEADER*>(sendBuf_.data())->size = 50004;

		for (int i = 0; i < 10000; ++i) {
			sprintf_s(sendBuf_.data() + sizeof(PACKET_HEADER) + i * 5, 6,
					  "%04d ", i);
		}

		memcpy(sendBuf_.data() + 50004, sendBuf_.data(), 50004);
		memcpy(sendBuf_.data() + 100008, sendBuf_.data(), 50004);

		int result;
		result = send(socket_, sendBuf_.data(), 150012, 0);
		if (result == SOCKET_ERROR) {
			success_ = false;
			LOG_ERROR("Failed to send data: {}", WSAGetLastError());
			return;
		}

		if (!ReceiveByte(recvBuf_.data(), 150012)) {
			success_ = false;
			LOG_ERROR("Failed to receive data: {}", WSAGetLastError());
			return;
		}
	}

	bool test() override {
		sendBuf_.fill(0);
		recvBuf_.fill(0);
		success_ = true;
		std::thread clientThread(&StickyPackets::ThreadFunc, this);
		clientThread.join();
		return success_ &&
			   memcmp(sendBuf_.data(), recvBuf_.data(), 150012) == 0;
	}
};
