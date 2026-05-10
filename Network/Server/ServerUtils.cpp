#include "ServerUtils.hpp"

#include <WinSock2.h>

#include "Network/Common/Logger.hpp"

namespace ServerUtils {
NetFuncs::NetFuncs() {
	SOCKET dummySocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr,
								   0, WSA_FLAG_OVERLAPPED);
	if (dummySocket == INVALID_SOCKET) {
		LOG_FATAL("[Error:{}] Failed to create dummy socket",
				  WSAGetLastError());
	}

	DWORD bytesReturned = 0;

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	int result =
		WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
				 sizeof(guidAcceptEx), static_cast<void*>(&AcceptEx),
				 sizeof(AcceptEx), &bytesReturned, nullptr, nullptr);

	if (result == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] WSAIoctl failed to get AcceptEx pointer",
				  WSAGetLastError());
	}

	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	result = WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
					  &guidDisconnectEx, sizeof(guidDisconnectEx),
					  static_cast<void*>(&DisconnectEx), sizeof(DisconnectEx),
					  &bytesReturned, nullptr, nullptr);

	if (result == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] WSAIoctl failed to get DisconnectEx pointer",
				  WSAGetLastError());
	}

	closesocket(dummySocket);
}

SOCKET CreateListenSocket(USHORT port) {
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr,
									0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		LOG_FATAL("[Error:{}] WSASocket failed", WSAGetLastError());
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr),
			 sizeof(serverAddr)) == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] bind failed", WSAGetLastError());
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] listen failed", WSAGetLastError());
	}

	return listenSocket;
}

LPFN_ACCEPTEX AcceptEx = nullptr;
LPFN_DISCONNECTEX DisconnectEx = nullptr;
}  // namespace ServerUtils
