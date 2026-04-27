#include "Client.hpp"

#include <ws2tcpip.h>

#include <cstddef>

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/Protocol.hpp"

Client::Client(const char* ip, const uint16_t port)
	: socket_(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
	if (socket_ == INVALID_SOCKET) {
		LOG_FATAL("Failed to create socket: {}", WSAGetLastError());
	}

	int opt = 1;
	if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
				   sizeof(opt)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(SO_REUSEADDR) failed: {}", WSAGetLastError());
	}

	int tcp_opt = 1;
	if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char*)&tcp_opt,
				   sizeof(tcp_opt)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt TCP_NODELAY failed: {}", WSAGetLastError());
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, ip, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(port);
	if (connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		closesocket(socket_);
		LOG_FATAL("connect failed: {}", WSAGetLastError());
	}
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
			LOG_INFO("Connection closed by server.");
			return false;
		}
		if (result == SOCKET_ERROR) {
			LOG_ERROR("send failed: {}", WSAGetLastError());
			return false;
		}
		bytesSent += result;
	}
	return true;
}

bool Client::ReceiveByte(char* buffer, const size_t len) {
	size_t bytesReceived = 0;
	while (bytesReceived < len) {
		int result =
			recv(socket_, buffer + bytesReceived, len - bytesReceived, 0);
		if (result == 0) {
			LOG_INFO("Connection closed by server.");
			return false;
		}
		if (result == SOCKET_ERROR) {
			LOG_ERROR("recv failed: {}", WSAGetLastError());
			return false;
		}
		bytesReceived += result;
	}
	return true;
}

bool Client::HandlePacket(const PACKET_HEADER* header) {
	if (header->id == static_cast<uint16_t>(C2S_PACKET_ID::MOVE)) {
	} else if (header->id == static_cast<uint16_t>(C2S_PACKET_ID::CHAT)) {
	} else {
		LOG_WARN("Unknown packet ID received.");
		return false;
	}

	return true;
}
