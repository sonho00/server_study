#include <Winsock2.h>

#include <iostream>
#include <thread>

#include "../Common/NetUtils.hpp"

#pragma comment(lib, "ws2_32.lib")

void handleClient(SOCKET clientSocket) {
	char buffer[1024];
	while (true) {
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0) {
			send(clientSocket, buffer, bytesReceived, 0);
		} else if (bytesReceived == 0) {
			// 클라이언트 연결 종료
			break;
		} else {
			NetUtils::PrintError("recv failed");
			break;
		}
	}

	closesocket(clientSocket);
}

int main() {
	std::cout << "Starting echo server..." << std::endl;

	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		NetUtils::PrintError("WSAStartup failed");
		return 1;
	}

	std::cout << "Echo server started on port 8080..." << std::endl;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		NetUtils::PrintError("socket failed");
		WSACleanup();
		return 1;
	}

	int opt = 1;
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
				   sizeof(opt)) == SOCKET_ERROR) {
		NetUtils::PrintError("setsockopt failed");
	}
	std::cout << "Socket created successfully." << std::endl;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(8080);
	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		NetUtils::PrintError("bind failed");
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Socket bound to port 8080." << std::endl;

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		NetUtils::PrintError("listen failed");
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Listening for incoming connections..." << std::endl;

	while (true) {
		sockaddr_in clientAddr;
		int clientAddrSize = sizeof(clientAddr);
		SOCKET clientSocket =
			accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);

		if (clientSocket == INVALID_SOCKET) {
			NetUtils::PrintError("accept failed");
			continue;
		}

		std::thread clientThread(handleClient, clientSocket);
		clientThread.detach();
	}

	closesocket(listenSocket);
	WSACleanup();

	std::cout << "Echo server stopped." << std::endl;

	return 0;
}
