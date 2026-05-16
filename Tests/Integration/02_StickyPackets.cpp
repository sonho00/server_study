#include <gtest/gtest.h>

#include <cstring>
#include <thread>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

class StickyPacketsTest : public testing::Test, public Client {
   public:
	void ThreadFunc() {
		Init();
		
		reinterpret_cast<PACKET_HEADER*>(sendBuf_.data())->size = 25004;
		reinterpret_cast<PACKET_HEADER*>(sendBuf_.data())->id =
			static_cast<uint16_t>(C2S_PACKET_ID::kChat);

		for (int i = 0; i < 5000; ++i) {
			sprintf_s(sendBuf_.data() + sizeof(PACKET_HEADER) + i * 5, 6,
					  "%04d ", i);
		}

		memcpy(sendBuf_.data() + 25004, sendBuf_.data(), 25004);
		memcpy(sendBuf_.data() + 50008, sendBuf_.data(), 25004);

		if (!SendByte(sendBuf_.data(), 75012)) {
			success_ = false;
			LOG_ERROR("Failed to send data: {}", WSAGetLastError());
			return;
		}

		if (!ReceiveByte(recvBuf_.data(), 75012)) {
			success_ = false;
			LOG_ERROR("Failed to receive data: {}", WSAGetLastError());
			return;
		}
	}

   protected:
	std::array<char, 76000> sendBuf_{};
	std::array<char, 76000> recvBuf_{};
};

TEST_F(StickyPacketsTest, VerifyDataIntegrity) {
	std::thread clientThread(&StickyPacketsTest::ThreadFunc, this);
	clientThread.join();

	EXPECT_TRUE(success_);
	EXPECT_EQ(memcmp(sendBuf_.data(), recvBuf_.data(), 75012), 0);
}
