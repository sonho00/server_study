#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "Client.hpp"
#include "Network/Common/Protocol.hpp"
#include "Network/Common/WSAManager.hpp"

#pragma comment(lib, "ws2_32.lib")

C2S_MOVE movePacket;
C2S_CHAT chatPacket;

int main(int argc, char* argv[]) {
	std::cout << "Starting echo client..." << std::endl;

	WSAManager wsaManager;

	std::cout << "Echo client started." << std::endl;

	const char* serverIp = "127.0.0.1";
	uint16_t serverPort = 8080;

	chatPacket.header.id = static_cast<uint16_t>(C2S_PACKET_ID::CHAT);
	chatPacket.header.size =
		sizeof(PACKET_HEADER) + sizeof("Hello, Echo Server!");
	strcpy_s(chatPacket.message, sizeof(chatPacket.message),
			 "Hello, Echo Server!");

	movePacket.header.id = static_cast<uint16_t>(C2S_PACKET_ID::MOVE);
	movePacket.header.size = sizeof(movePacket);
	movePacket.x = 1.0f;
	movePacket.y = 2.0f;

	std::cout << "Creating clients and connecting to server..." << std::endl;

	std::vector<Client> clients;
	clients.reserve(4000);
	for (int i = 0; i < (argc > 1 ? std::stoi(argv[1]) : 4000); ++i) {
		try {
			clients.emplace_back(serverIp, serverPort);
			std::thread(&Client::ThreadFunc, &clients.back()).detach();
		} catch (const std::exception& ex) {
			std::cerr << "Failed to create client: " << ex.what() << std::endl;
			break;
		}
	}

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	std::cout << "Press Enter to stop the client..." << std::endl;
	std::cin.get();

	std::cout << "Echo client stopped." << std::endl;

	return 0;
}
