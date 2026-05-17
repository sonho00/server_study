#include <gtest/gtest.h>
#include <string.h>

#include "Tests/Base/Client.hpp"

TEST(BroadcastTest, BroadcastMessageToAllClients) {
	Client client1, client2, client3;
	client1.Init();
	client2.Init();
	client3.Init();

	C2S_CHAT sendPacket;
	strcpy_s(sendPacket.message, "Hello, everyone!");
	sendPacket.header.id = static_cast<uint16_t>(C2S_PACKET_ID::kChat);
	sendPacket.header.size =
		sizeof(sendPacket.header) + strlen(sendPacket.message) + 1;
	EXPECT_TRUE(client3.SendByte(reinterpret_cast<char*>(&sendPacket),
								 sendPacket.header.size));

	S2C_CHAT recvPacket1, recvPacket2;
	EXPECT_TRUE(client1.ReceivePacket(reinterpret_cast<char*>(&recvPacket1)));
	EXPECT_TRUE(client2.ReceivePacket(reinterpret_cast<char*>(&recvPacket2)));

	size_t expectedSize =
		sendPacket.header.size + sizeof(uint64_t);	// 세션 ID 추가
	EXPECT_EQ(recvPacket1.header.id,
			  static_cast<uint16_t>(S2C_PACKET_ID::kChat));
	EXPECT_EQ(recvPacket2.header.id,
			  static_cast<uint16_t>(S2C_PACKET_ID::kChat));
	EXPECT_EQ(recvPacket1.header.size, expectedSize);
	EXPECT_EQ(recvPacket2.header.size, expectedSize);
	EXPECT_STREQ(recvPacket1.message, "Hello, everyone!");
	EXPECT_STREQ(recvPacket2.message, "Hello, everyone!");

	unsigned long mode = 1;
	ioctlsocket(client3.socket_, FIONBIO, &mode);

	char test;
	LOG_INFO(
		"Attempting to receive from client3 (should fail due to non-blocking "
		"mode)...");
	EXPECT_FALSE(client3.ReceiveByte(&test, 1));
}
