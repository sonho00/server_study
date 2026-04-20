#include <WinSock2.h>
#include <mswsock.h>

#include <thread>

#include "IocpCore.hpp"
#include "NetUtils.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
	if (!NetUtils::Init()) {
		return 1;
	}

	SOCKET listenSocket = NetUtils::CreateListenSocket(8080);
	if (listenSocket == INVALID_SOCKET) {
		NetUtils::PrintError("Failed to create listen socket");
		WSACleanup();
		return 1;
	}

	IocpCore iocp;

	if (!iocp.Init(listenSocket)) {
		NetUtils::PrintError("Failed to initialize IOCP core");
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (!iocp.Start(std::thread::hardware_concurrency())) {
		NetUtils::PrintError("Failed to start IOCP worker threads");
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (!iocp.PostAccept()) {
		NetUtils::PrintError("Failed to post initial accept");
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	Sleep(INFINITE);

	closesocket(listenSocket);
	WSACleanup();
}
