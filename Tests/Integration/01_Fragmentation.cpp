#include <gtest/gtest.h>

#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

TEST(FragmentationTest, HandleFragmentedPacket) {
	Client client1, client2;
	client1.Init();
	client2.Init();

	std::array<char, 404> sendBuf{};
	std::array<char, 404> recvBuf{};
	auto* sendHeader = reinterpret_cast<PACKET_HEADER*>(sendBuf.data());

	sendHeader->id = static_cast<uint16_t>(C2S_PACKET_ID::kChat);
	sendHeader->size = 404;

	for (int i = 0; i < 100; ++i) {
		sprintf_s(sendBuf.data() + sizeof(PACKET_HEADER) + i * 4, 5, "%03d ",
				  i);
	}

	for (int i = 0; i < 404; ++i) {
		EXPECT_TRUE(client1.SendByte(sendBuf.data() + i, 1));
	}

	S2C_CHAT recvPacket;
	EXPECT_TRUE(client2.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));

	EXPECT_EQ(recvPacket.header.id,
			  static_cast<uint16_t>(S2C_PACKET_ID::kChat));
	EXPECT_EQ(recvPacket.header.size,
			  sendHeader->size + sizeof(recvPacket.sessionHandle));
	EXPECT_STREQ(recvPacket.message, sendBuf.data() + sizeof(PACKET_HEADER));
}
