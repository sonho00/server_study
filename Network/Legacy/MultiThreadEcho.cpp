#include <Winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <thread>

#include "../Common/NetUtils.hpp"
#include "Network/Common/WSAManager.hpp"

#pragma comment(lib, "ws2_32.lib")

void handleClient(SOCKET clientSocket) {
	char buffer[1024];
	while (true) {
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0) {
			if (send(clientSocket, buffer, bytesReceived, 0) == SOCKET_ERROR) {
				LOG_ERROR("send failed: {}", WSAGetLastError());
				break;
			} else if (bytesReceived == 0) {
				// 클라이언트 연결 종료
				LOG_INFO("Connection closed by client.");
				break;
			} else {
				LOG_ERROR("recv failed: {}", WSAGetLastError());
				break;
			}
		} else if (bytesReceived == 0) {
			// 클라이언트 연결 종료
			LOG_INFO("Connection closed by client.");
			break;
		} else {
			LOG_ERROR("recv failed: {}", WSAGetLastError());
			break;
		}
	}

	closesocket(clientSocket);
}

int main() {
	LOG_INFO("Starting echo server on port 8080...");

	WSAManager wsaManager;

	LOG_INFO("WSA initialized successfully.");

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		LOG_FATAL("socket failed: {}", WSAGetLastError());
	}

	int opt = 1;
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
				   sizeof(opt)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(SO_REUSEADDR) failed: {}", WSAGetLastError());
	}
	LOG_INFO("Socket options set successfully.");

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(8080);
	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		LOG_FATAL("bind failed: {}", WSAGetLastError());
	}

	LOG_INFO("Socket bound to port 8080.");

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		LOG_FATAL("listen failed: {}", WSAGetLastError());
	}

	LOG_INFO("Socket is listening for incoming connections.");

	std::thread serverThread([&]() {
		while (true) {
			sockaddr_in clientAddr;
			int clientAddrSize = sizeof(clientAddr);
			SOCKET clientSocket =
				accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);

			if (clientSocket != INVALID_SOCKET) {
				char ip[INET_ADDRSTRLEN];
				InetNtop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
				LOG_INFO("Accepted connection from {}:{}", ip,
						 ntohs(clientAddr.sin_port));
			} else {
				LOG_ERROR("accept failed: {}", WSAGetLastError());
			}

			std::thread clientThread(handleClient, clientSocket);
			clientThread.detach();
		}
	});

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	LOG_INFO("Press Enter to stop the server...");
	std::cin.get();
	LOG_INFO("Server is shutting down...");

	closesocket(listenSocket);

	LOG_INFO("Echo server stopped.");

	return 0;
}
