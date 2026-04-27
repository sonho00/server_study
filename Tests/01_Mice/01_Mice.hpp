#pragma once

#include <stdio.h>

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class Mice : public Client {
   public:
	Mice(const char* ip, const uint16_t port) : Client(ip, port) {}
	void ThreadFunc() override {
		char buf[605]{};
		C2S_CHAT* packet = reinterpret_cast<C2S_CHAT*>(buf);
		packet->header.id = static_cast<uint16_t>(C2S_PACKET_ID::CHAT);
		packet->header.size = 604;
		char temp[4]{};
		for (int i = 0; i < 200; ++i) {
			sprintf_s(temp, 4, "%03d", i);
			memcpy(packet->message + i * 3, temp, 3);
		}
		for (int i = 0; i < 605; ++i) {
			int result = send(socket_, buf + i, 1, 0);
			if (result == 0) {
				LOG_INFO("Connection closed by server.");
				return;
			}
		}
		memset(buf, 0, 605);
		std::cout << "Reset buffer: " << packet->header.id << " "
				  << packet->header.size << " " << packet->message << std::endl;
		recv(socket_, buf, 605, 0);
		std::cout << packet->header.id << " " << packet->header.size << " "
				  << packet->message << std::endl;
	}
};
