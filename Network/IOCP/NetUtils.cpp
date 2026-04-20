#include "NetUtils.hpp"

namespace NetUtils {
bool Init() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		PrintError("WSAStartup failed");
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

LPFN_ACCEPTEX GetAcceptEx(SOCKET listen_socket) {
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	LPFN_ACCEPTEX lpfnAcceptEx = nullptr;
	DWORD bytesReturned = 0;

	int result = WSAIoctl(listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
						  &guidAcceptEx, sizeof(guidAcceptEx), &lpfnAcceptEx,
						  sizeof(lpfnAcceptEx), &bytesReturned, NULL, NULL);

	if (result == SOCKET_ERROR) {
		PrintError("WSAIoctl failed to get AcceptEx pointer");
		return nullptr;
	}

	return lpfnAcceptEx;
}
}  // namespace NetUtils
