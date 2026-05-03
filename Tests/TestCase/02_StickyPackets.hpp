#pragma once

#include <cstdio>

#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class StickyPackets : public Client {
   public:
	StickyPackets(const char* ip, const uint16_t port) : Client(ip, port) {}
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
		send(socket_, sendBuf_.data(), 150012, 0);
		recv(socket_, recvBuf_.data(), 150012, 0);
	}
};
