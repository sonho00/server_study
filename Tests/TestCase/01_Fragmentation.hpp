#pragma once

#include <cstdio>
#include <cstring>
#include <thread>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class Fragmentation : public Client {
   public:
	void ThreadFunc() override {
		auto* packet = reinterpret_cast<C2S_CHAT*>(sendBuf_.data());
		packet->header.id = static_cast<uint16_t>(C2S_PACKET_ID::kChat);
		packet->header.size = 404;

		for (int i = 0; i < 100; ++i) {
			sprintf_s(packet->message + i * 4, 5, "%03d ", i);
		}

		for (int i = 0; i < 404; ++i) {
			if (!SendByte(sendBuf_.data() + i, 1)) {
				success_ = false;
				LOG_ERROR("Failed to send data: {}", WSAGetLastError());
				return;
			}
		}

		if (!ReceiveByte(recvBuf_.data(), packet->header.size)) {
			success_ = false;
			LOG_ERROR("Failed to receive data: {}", WSAGetLastError());
			return;
		}
	}

	bool test() override {
		sendBuf_.fill(0);
		recvBuf_.fill(0);
		success_ = true;
		std::thread clientThread(&Fragmentation::ThreadFunc, this);
		clientThread.join();
		return success_ && memcmp(sendBuf_.data(), recvBuf_.data(), 404) == 0;
	}
};
