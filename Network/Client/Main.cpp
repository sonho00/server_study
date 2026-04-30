#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "Client.hpp"
#include "Network/Common/Config.hpp"
#include "Network/Common/Protocol.hpp"
#include "Network/Common/WSAManager.hpp"

C2S_MOVE movePacket;
C2S_CHAT chatPacket;

int main(int argc, char* argv[]) {
	uint32_t clientCount = argc > 1 ? std::stoi(argv[1]) : Config::kClientCount;

	LOG_INFO("Starting echo client with {} clients...", clientCount);

	WSAManager wsaManager;

	LOG_INFO("Echo client started.");

	chatPacket.header.id = static_cast<uint16_t>(C2S_PACKET_ID::kChat);
	chatPacket.header.size =
		sizeof(PACKET_HEADER) + sizeof("Hello, Echo Server!");
	strcpy_s(chatPacket.message, sizeof(chatPacket.message),
			 "Hello, Echo Server!");

	movePacket.header.id = static_cast<uint16_t>(C2S_PACKET_ID::kMove);
	movePacket.header.size = sizeof(movePacket);
	movePacket.x = 1.0F;  // NOLINT(readability-magic-numbers)
	movePacket.y = 2.0F;  // NOLINT(readability-magic-numbers)

	LOG_INFO("Creating clients and connecting to server...");

	const char* serverIp = "127.0.0.1";
	uint16_t serverPort = Config::kPort;
	std::vector<Client> clients;
	std::vector<std::thread> threads;
	threads.reserve(clientCount);
	clients.reserve(clientCount);

	for (size_t i = 0; i < clientCount; ++i) {
		try {
			clients.emplace_back(serverIp, serverPort);
		} catch (const std::exception& ex) {
			LOG_ERROR("Failed to create client {}: {}", i, ex.what());
			break;
		}
	}

	for (size_t i = 0; i < clientCount; ++i) {
		threads.emplace_back(&Client::ThreadFunc, &clients[i]);
	}

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	LOG_INFO("Press Enter to stop the client...");
	std::cin.get();

	for (auto& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	LOG_INFO("Echo client stopped.");

	return 0;
}
