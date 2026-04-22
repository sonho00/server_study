#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <thread>

#include "Protocol.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
	std::cout << "Starting echo client..." << std::endl;

	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
		return 1;
	}

	std::cout << "Echo client started." << std::endl;

	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	std::cout << "Socket created successfully." << std::endl;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = htons(8080);
	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Connected to echo server." << std::endl;

	C2S_CHAT chatPacket;
	chatPacket.header.id = C2S_PACKET_ID::CHAT;
	chatPacket.header.size =
		sizeof(PACKET_HEADER) + sizeof("Hello, Echo Server!");
	strcpy_s(chatPacket.message, sizeof(chatPacket.message),
			 "Hello, Echo Server!");

	std::thread thread([clientSocket, chatPacket]() {
		while (true) {
			int bytesSent =
				send(clientSocket, reinterpret_cast<const char*>(&chatPacket),
					 chatPacket.header.size, 0);
			if (bytesSent == SOCKET_ERROR) {
				std::cerr << "send failed: " << WSAGetLastError() << std::endl;
				closesocket(clientSocket);
				WSACleanup();
				break;
			}

			std::cout << "Message sent to server: " << chatPacket.message
					  << std::endl;

			char buffer[1024];
			int bytesReceived =
				recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
			if (bytesReceived == SOCKET_ERROR) {
				std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
				closesocket(clientSocket);
				WSACleanup();
				break;
			}
			buffer[bytesReceived] = '\0';

			std::cout << "Message received from server: "
					  << buffer + sizeof(PACKET_HEADER) << std::endl;

			Sleep(1000);
		}
	});

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	std::cout << "Press Enter to stop the client..." << std::endl;
	std::cin.get();

	std::cout << "Closing connection..." << std::endl;

	closesocket(clientSocket);
	WSACleanup();

	std::cout << "Echo client stopped." << std::endl;

	return 0;
}
