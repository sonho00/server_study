#include <gtest/gtest.h>

#include <cstring>

#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

TEST(StickyPacketsTest, VerifyDataIntegrity) {
	Client client1, client2;
	client1.Init();
	client2.Init();

	std::array<char, 76000> sendBuf{};
	std::array<char, 76000> recvBuf{};
	auto* sendHeader = reinterpret_cast<PACKET_HEADER*>(sendBuf.data());

	sendHeader->size = 25004;
	sendHeader->id = static_cast<uint16_t>(C2S_PACKET_ID::kChat);

	for (int i = 0; i < 5000; ++i) {
		sprintf_s(sendBuf.data() + sizeof(PACKET_HEADER) + i * 5, 6, "%04d ",
				  i);
	}

	memcpy(sendBuf.data() + 25004, sendBuf.data(), 25004);
	memcpy(sendBuf.data() + 50008, sendBuf.data(), 25004);

	EXPECT_TRUE(client1.SendByte(sendBuf.data(), 75012));
	EXPECT_TRUE(client2.ReceiveByte(recvBuf.data(), 75036));

	for (int i = 0; i < 3; ++i) {
		auto* recvHeader =
			reinterpret_cast<PACKET_HEADER*>(recvBuf.data() + i * 25012);
		EXPECT_EQ(recvHeader->id, static_cast<uint16_t>(C2S_PACKET_ID::kChat));
		EXPECT_EQ(recvHeader->size, 25012);

		EXPECT_EQ(memcmp(recvBuf.data() + i * 25012 + sizeof(PACKET_HEADER) +
							 sizeof(uint64_t),
						 sendBuf.data() + sizeof(PACKET_HEADER), 25000),
				  0);
	}
}
