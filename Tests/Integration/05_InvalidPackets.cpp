#include <gtest/gtest.h>

#include <cstdint>

#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

TEST(InvalidPackets, InvalidHeader) {
	Client client;
	client.Init();

	PACKET_HEADER invalidHeader{};
	invalidHeader.size = sizeof(PACKET_HEADER) - 1;	 // Invalid size
	EXPECT_TRUE(client.SendByte((char*)&invalidHeader, sizeof(invalidHeader)));

	char buffer[1024];

	// 서버에서 잘못된 패킷 크기를 감지하여 연결을 종료할 것으로 예상
	EXPECT_FALSE(client.ReceiveByte(buffer, sizeof(PACKET_HEADER)));

	invalidHeader.size = sizeof(PACKET_HEADER);
	invalidHeader.id = static_cast<uint16_t>(-1);  // Invalid packet ID
	EXPECT_TRUE(client.SendByte((char*)&invalidHeader, sizeof(invalidHeader)));

	// 서버에서 잘못된 패킷 ID를 감지하여 연결을 종료할 것으로 예상
	EXPECT_FALSE(client.ReceiveByte(buffer, sizeof(PACKET_HEADER)));
}
