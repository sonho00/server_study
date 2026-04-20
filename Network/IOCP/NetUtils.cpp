#include "NetUtils.hpp"

#include <WinSock2.h>

namespace NetUtils {
bool Init() {
	if (!GetAcceptEx()) {
		PrintError("Failed to get AcceptEx function pointer");
		WSACleanup();
		return false;
	}

	return true;
}

SOCKET CreateListenSocket(USHORT port) {
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
									WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		PrintError("WSASocket failed");
		return INVALID_SOCKET;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		PrintError("bind failed");
		closesocket(listenSocket);
		return INVALID_SOCKET;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		PrintError("listen failed");
		closesocket(listenSocket);
		return INVALID_SOCKET;
	}

	return listenSocket;
}

bool GetAcceptEx() {
	SOCKET dummySocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
								   WSA_FLAG_OVERLAPPED);
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	LPFN_ACCEPTEX lpfnAcceptEx = nullptr;
	DWORD bytesReturned = 0;

	int result = WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
						  &guidAcceptEx, sizeof(guidAcceptEx), &lpfnAcceptEx,
						  sizeof(lpfnAcceptEx), &bytesReturned, NULL, NULL);

	if (result == SOCKET_ERROR) {
		PrintError("WSAIoctl failed to get AcceptEx pointer");
		return false;
	}

	AcceptEx = lpfnAcceptEx;
	return true;
}

LPFN_ACCEPTEX AcceptEx = nullptr;
}  // namespace NetUtils
