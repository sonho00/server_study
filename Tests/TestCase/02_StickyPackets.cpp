#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <thread>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Client.hpp"

class StickyPackets : public Client {
   public:
	StickyPackets() {
		CreateSocket();
		DefaultSockOpt();
		Connect();
	}

	void ThreadFunc() override {
		reinterpret_cast<PACKET_HEADER*>(sendBuf_.data())->size = 50004;
		reinterpret_cast<PACKET_HEADER*>(sendBuf_.data())->id =
			static_cast<uint16_t>(C2S_PACKET_ID::kChat);

		for (int i = 0; i < 10000; ++i) {
			sprintf_s(sendBuf_.data() + sizeof(PACKET_HEADER) + i * 5, 6,
					  "%04d ", i);
		}

		int result;
		for (int i = 0; i < 3; ++i) {
			result = SendPacket(reinterpret_cast<const PACKET_HEADER&>(
				sendBuf_.data()[i * 50004]));
			if (result == SOCKET_ERROR) {
				success_ = false;
				LOG_ERROR("Failed to send data: {}", WSAGetLastError());
				return;
			}
			if (i < 2) {
				memcpy(sendBuf_.data() + (i + 1) * 50004, sendBuf_.data(),
					   50004);
			}
			Sleep(1);
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

   private:
	std::array<char, 160000> sendBuf_;
	std::array<char, 160000> recvBuf_;
};

TEST(NetworkTest, StickyPackets) {
	StickyPackets client;
	EXPECT_TRUE(client.test());
}
