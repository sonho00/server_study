#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <thread>

#include "ClientNetwork.hpp"
#include "Network/Common/Protocol.hpp"
#include "Network/Common/WSAManager.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
	std::cout << "Starting echo client..." << std::endl;

	WSAManager wsaManager;

	std::cout << "Echo client started." << std::endl;

	const char* serverIp = "127.0.0.1";
	uint16_t serverPort = 8080;
	Client client(serverIp, serverPort);

	std::cout << "Connected to echo server." << std::endl;

	C2S_CHAT chatPacket;
	chatPacket.header.id = static_cast<uint16_t>(C2S_PACKET_ID::CHAT);
	chatPacket.header.size =
		sizeof(PACKET_HEADER) + sizeof("Hello, Echo Server!");
	strcpy_s(chatPacket.message, sizeof(chatPacket.message),
			 "Hello, Echo Server!");

	C2S_MOVE movePacket;
	movePacket.header.id = static_cast<uint16_t>(C2S_PACKET_ID::MOVE);
	movePacket.header.size = sizeof(movePacket);
	movePacket.x = 1.0f;
	movePacket.y = 2.0f;

	char buffer[1024];

	std::thread sendThread([&client, chatPacket, movePacket]() {
		while (true) {
			if (!SendPacket(client.socket_, &chatPacket.header)) {
				std::cerr << "Failed to send packet to server." << std::endl;
				break;
			}

			if (!SendPacket(client.socket_, &movePacket.header)) {
				std::cerr << "Failed to send packet to server." << std::endl;
				break;
			}

			std::cout << "Message sent to server: " << chatPacket.message
					  << std::endl;

			Sleep(1000);
		}
	});

	std::thread receiveThread([&client, &buffer]() {
		while (true) {
			if (!ReceivePacket(client.socket_, buffer, sizeof(PACKET_HEADER))) {
				std::cerr << "Failed to receive packet from server."
						  << std::endl;
				break;
			}
			
			PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(buffer);
			if (!ReceivePacket(client.socket_, buffer + sizeof(PACKET_HEADER),
							   header->size - sizeof(PACKET_HEADER))) {
				std::cerr << "Failed to receive full packet from server."
						  << std::endl;
				break;
			}

			if (!HandlePacket(header)) {
				std::cerr << "Failed to handle packet from server."
						  << std::endl;
				break;
			}
		}
	});

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	std::cout << "Press Enter to stop the client..." << std::endl;
	std::cin.get();

	std::cout << "Echo client stopped." << std::endl;

	return 0;
}
