#pragma once

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class Fragmentation : public Client {
   public:
	Fragmentation(const char* ip, const uint16_t port) : Client(ip, port) {}
	void ThreadFunc() override {
		char buf[405]{};
		auto* packet = reinterpret_cast<C2S_CHAT*>(buf);
		packet->header.id = static_cast<uint16_t>(C2S_PACKET_ID::kChat);
		packet->header.size = 404;
		char temp[5]{};

		for (int i = 0; i < 100; ++i) {
			sprintf_s(temp, 5, "%03d ", i);
			memcpy(packet->message + i * 4, temp, 4);
		}
		for (int i = 0; i < 405; ++i) {
			int result = send(socket_, buf + i, 1, 0);
			if (result == 0) {
				LOG_INFO("Connection closed by server.");
				return;
			}
		}

		memset(buf, 0, 405);
		std::cout << std::format("Reset buffer: {} {} {}\n", packet->header.id,
								 packet->header.size, packet->message);
		recv(socket_, buf, 405, 0);
		std::cout << std::format("{} {} {}", packet->header.id,
								 packet->header.size, packet->message);
	}
};
