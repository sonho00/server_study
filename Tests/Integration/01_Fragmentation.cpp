#include <gtest/gtest.h>

#include <cstring>
#include <thread>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

class FragmentationTest : public testing::Test, public Client {
   public:
	void ThreadFunc() {
		Init();

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

   protected:
	std::array<char, 500> sendBuf_{};
	std::array<char, 500> recvBuf_{};
};

TEST_F(FragmentationTest, VerifyDataIntegrity) {
	std::thread clientThread(&FragmentationTest::ThreadFunc, this);
	clientThread.join();

	EXPECT_TRUE(success_);
	EXPECT_EQ(memcmp(sendBuf_.data(), recvBuf_.data(), 404), 0);
}
