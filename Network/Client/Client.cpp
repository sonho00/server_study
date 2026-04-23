#include "Client.hpp"

#include <ws2tcpip.h>

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

#include "Network/Common/Protocol.hpp"

Client::Client(const char* ip, const uint16_t port)
	: socket_(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
	if (socket_ == INVALID_SOCKET) {
		throw std::runtime_error("socket failed: " +
								 std::to_string(WSAGetLastError()));
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, ip, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(port);
	if (connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		closesocket(socket_);
		throw std::runtime_error("connect failed: " +
								 std::to_string(WSAGetLastError()));
	}

	std::cout << "Connected to server at " << ip << ":" << port << std::endl;
}

Client::~Client() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
	}
}

bool Client::SendPacket(const PACKET_HEADER* header) {
	const char* buffer = reinterpret_cast<const char*>(header);
	int bytesSent = 0;
	while (bytesSent < header->size) {
		int result =
			send(socket_, buffer + bytesSent, header->size - bytesSent, 0);
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

bool Client::ReceivePacket(char* buffer, const size_t len) {
	size_t bytesReceived = 0;
	while (bytesReceived < len) {
		int result =
			recv(socket_, buffer + bytesReceived, len - bytesReceived, 0);
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

bool Client::HandlePacket(const PACKET_HEADER* header) {
	std::cout << "Received packet from server: ID=" << header->id
			  << " Size=" << header->size << std::endl;

	if (header->id == static_cast<uint16_t>(C2S_PACKET_ID::MOVE)) {
		const C2S_MOVE* moveResponse =
			reinterpret_cast<const C2S_MOVE*>(header);
		std::cout << "Move packet from server: x=" << moveResponse->x
				  << " y=" << moveResponse->y << std::endl;
	} else if (header->id == static_cast<uint16_t>(C2S_PACKET_ID::CHAT)) {
		const C2S_CHAT* chatResponse =
			reinterpret_cast<const C2S_CHAT*>(header);
		std::cout << "Chat packet from server: message="
				  << std::string_view(chatResponse->message,
									  header->size - sizeof(PACKET_HEADER))
				  << std::endl;
	} else {
		std::cout << "Unknown packet ID received." << std::endl;
		return false;
	}

	return true;
}
