#include "ClientNetwork.hpp"

#include <ws2tcpip.h>

#include <cstddef>
#include <iostream>

#include "Network/Common/Protocol.hpp"

SOCKET CreateClientSocket(const char* ip, uint16_t port) {
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
		return INVALID_SOCKET;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, ip, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(port);
	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
		closesocket(clientSocket);
		return INVALID_SOCKET;
	}

	std::cout << "Connected to server at " << ip << ":" << port << std::endl;
	return clientSocket;
}

Client::Client(const char* ip, uint16_t port) {
	socket_ = CreateClientSocket(ip, port);
	if (socket_ == INVALID_SOCKET)
		throw std::runtime_error("Failed to create client socket");
}

Client::~Client() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
	}
}

bool SendPacket(SOCKET clientSocket, const PACKET_HEADER* header) {
	const char* buffer = reinterpret_cast<const char*>(header);
	int bytesSent = 0;
	while (bytesSent < header->size) {
		int result =
			send(clientSocket, buffer + bytesSent, header->size - bytesSent, 0);
		if (result == 0) {
			std::cerr << "Connection closed by server." << std::endl;
			return false;
		}
		if (result == SOCKET_ERROR) {
			std::cerr << "send failed: " << WSAGetLastError() << std::endl;
			return false;
		}
		bytesSent += result;
	}
	return true;
}

bool ReceivePacket(SOCKET clientSocket, char* buffer, size_t len) {
	size_t bytesReceived = 0;
	while (bytesReceived < len) {
		int result =
			recv(clientSocket, buffer + bytesReceived, len - bytesReceived, 0);
		if (result == 0) {
			std::cerr << "Connection closed by server." << std::endl;
			return false;
		}
		if (result == SOCKET_ERROR) {
			std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
			return false;
		}
		bytesReceived += result;
		if (bytesReceived >= sizeof(PACKET_HEADER)) {
			PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(buffer);
			if (bytesReceived >= header->size) {
				break;
			}
		}
	}
	return true;
}

bool HandlePacket(PACKET_HEADER* header) {
	std::cout << "Received packet from server: ID=" << header->id
			  << " Size=" << header->size << std::endl;

	if (header->id == static_cast<uint16_t>(C2S_PACKET_ID::MOVE)) {
		C2S_MOVE* moveResponse = reinterpret_cast<C2S_MOVE*>(header);
		std::cout << "Move packet from server: x=" << moveResponse->x
				  << " y=" << moveResponse->y << std::endl;
	} else if (header->id == static_cast<uint16_t>(C2S_PACKET_ID::CHAT)) {
		C2S_CHAT* chatResponse = reinterpret_cast<C2S_CHAT*>(header);
		chatResponse->message[header->size - sizeof(PACKET_HEADER)] = '\0';
		std::cout << "Chat packet from server: message="
				  << chatResponse->message << std::endl;
	} else {
		std::cout << "Unknown packet ID received." << std::endl;
		return false;
	}

	return true;
}
