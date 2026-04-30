#include "ServerUtils.hpp"

#include <WinSock2.h>

#include "Network/Common/NetUtils.hpp"

namespace ServerUtils {
NetFuncs::NetFuncs() : acceptEx_(nullptr) {
	SOCKET dummySocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr,
								   0, WSA_FLAG_OVERLAPPED);

	if (dummySocket == INVALID_SOCKET) {
		LOG_FATAL("Failed to create dummy socket: {}", WSAGetLastError());
	}

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD bytesReturned = 0;

	int result =
		WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
				 sizeof(guidAcceptEx), static_cast<void*>(&acceptEx_),
				 sizeof(acceptEx_), &bytesReturned, nullptr, nullptr);

	closesocket(dummySocket);

	if (result == SOCKET_ERROR) {
		LOG_FATAL("WSAIoctl failed to get AcceptEx pointer: {}",
				  WSAGetLastError());
	}
}

SOCKET CreateListenSocket(USHORT port) {
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr,
									0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		LOG_FATAL("WSASocket failed: {}", WSAGetLastError());
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr),
			 sizeof(serverAddr)) == SOCKET_ERROR) {
		LOG_FATAL("bind failed: {}", WSAGetLastError());
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		LOG_FATAL("listen failed: {}", WSAGetLastError());
	}

	return listenSocket;
}
}  // namespace ServerUtils
