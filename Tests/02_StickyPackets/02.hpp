#pragma once

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class StickyPackets : public Client {
   public:
	StickyPackets(const char* ip, const uint16_t port) : Client(ip, port) {}
	void ThreadFunc() override {
		char buf[150013]{};
		reinterpret_cast<PACKET_HEADER*>(buf)->id =
			static_cast<uint16_t>(C2S_PACKET_ID::kChat);
		reinterpret_cast<PACKET_HEADER*>(buf)->size = 50004;
		char temp[6]{};
		for (int i = 0; i < 10000; ++i) {
			sprintf_s(temp, 6, "%04d ", i);
			memcpy(buf + sizeof(PACKET_HEADER) + i * 5, temp, 5);
		}
		memcpy(buf + 50004, buf, 50004);
		memcpy(buf + 100008, buf, 50004);
		send(socket_, buf, 150012, 0);

		memset(buf, 0, sizeof(buf));
		std::cout << std::format(
			"Reset buffer: {} {} {}\n",
			reinterpret_cast<PACKET_HEADER*>(buf)->id,
			reinterpret_cast<PACKET_HEADER*>(buf)->size,
			reinterpret_cast<char*>(buf + sizeof(PACKET_HEADER)));

		for (int i = 0; i < 3; ++i) {
			recv(socket_, buf + i * 50004, 50004, 0);
			std::cout << std::format(
				"{} {}\n",
				reinterpret_cast<PACKET_HEADER*>(buf + i * 50004)->id,
				reinterpret_cast<PACKET_HEADER*>(buf + i * 50004)->size);
		}
	}
};
