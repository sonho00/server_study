#include "Client.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"

Client::~Client() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
	}
}

void Client::CreateSocket() {
	socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_ == INVALID_SOCKET) {
		LOG_FATAL("Failed to create socket: {}", WSAGetLastError());
	}
}

bool Client::SendByte(char* buffer, int len) const {
	LOG_DEBUG("Attempting to send {} bytes to server", len);
	int bytesSent = 0;
	while (bytesSent < len) {
		int result = send(socket_, buffer + bytesSent, len - bytesSent, 0);
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

bool Client::ReceiveByte(char* buffer, int len) const {
	int bytesReceived = 0;
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

bool Client::SendPacket(const PACKET_HEADER& header) const {
	if (!SendByte((char*)&header, header.size)) {
		LOG_ERROR("Failed to send packet header.");
		return false;
	}
	return true;
}

PACKET_HEADER* Client::ReceivePacket(char* buffer) {
	if (!ReceiveByte(buffer, sizeof(PACKET_HEADER))) {
		LOG_WARN("Failed to receive packet header from server.");
		return nullptr;
	}

	auto* header = reinterpret_cast<PACKET_HEADER*>(buffer);

	if (!ReceiveByte(buffer + sizeof(PACKET_HEADER),
					 header->size - sizeof(PACKET_HEADER))) {
		LOG_WARN("Failed to receive full packet from server.");
		return nullptr;
	}

	return header;
}

bool Client::HandlePacket(const PACKET_HEADER& header) {
	switch (header.id) {
		case static_cast<uint16_t>(C2S_PACKET_ID::kMove):
		case static_cast<uint16_t>(C2S_PACKET_ID::kChat):
			break;

		default:
			LOG_WARN("Unknown packet ID received.");
			return false;
	}

	return true;
}

void Client::DefaultSockOpt() {
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
}

int Client::Connect() {
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, kIpAddr_, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(kPort_);
	if (connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		LOG_ERROR("Failed to connect to server: {}", errorCode);
		closesocket(socket_);
		return errorCode;
	}
	return 0;
}
