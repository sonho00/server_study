#include "ServerUtils.hpp"

#include <WinSock2.h>

#include "Network/Common/NetUtils.hpp"

namespace ServerUtils {
NetFuncs::NetFuncs() {
	SOCKET dummySocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
								   WSA_FLAG_OVERLAPPED);

	if (dummySocket == INVALID_SOCKET) {
		throw std::runtime_error("Failed to create dummy socket for AcceptEx");
	}

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD bytesReturned = 0;

	int result = WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
						  &guidAcceptEx, sizeof(guidAcceptEx), &AcceptEx,
						  sizeof(AcceptEx), &bytesReturned, NULL, NULL);

	closesocket(dummySocket);

	if (result == SOCKET_ERROR) {
		throw std::runtime_error("Failed to get AcceptEx function pointer");
	}
}

SOCKET CreateListenSocket(const USHORT port) {
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
									WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		NetUtils::PrintError("WSASocket failed");
		return INVALID_SOCKET;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr),
			 sizeof(serverAddr)) == SOCKET_ERROR) {
		NetUtils::PrintError("bind failed");
		closesocket(listenSocket);
		return INVALID_SOCKET;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		NetUtils::PrintError("listen failed");
		closesocket(listenSocket);
		return INVALID_SOCKET;
	}

	return listenSocket;
}
}  // namespace ServerUtils
