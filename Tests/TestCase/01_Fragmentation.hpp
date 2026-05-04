#pragma once

#include <cstdio>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class Fragmentation : public Client {
   public:
	Fragmentation(const char* ip, const uint16_t port) : Client(ip, port) {}
	void ThreadFunc() override {
		testBytes_ = 404;
		auto* packet = reinterpret_cast<C2S_CHAT*>(sendBuf_.data());
		packet->header.id = static_cast<uint16_t>(C2S_PACKET_ID::kChat);
		packet->header.size = 404;

		for (int i = 0; i < 100; ++i) {
			sprintf_s(packet->message + i * 4, 5, "%03d ", i);
		}

		for (int i = 0; i < 404; ++i) {
			int result = send(socket_, sendBuf_.data() + i, 1, 0);
			if (result == SOCKET_ERROR) {
				success_ = false;
				LOG_ERROR("Failed to send byte {}: {}", i, WSAGetLastError());
				return;
			}
		}

		int result = recv(socket_, recvBuf_.data(), 404, 0);
		if (result == SOCKET_ERROR) {
			success_ = false;
			LOG_ERROR("Failed to receive data: {}", WSAGetLastError());
			return;
		}
	}
};
