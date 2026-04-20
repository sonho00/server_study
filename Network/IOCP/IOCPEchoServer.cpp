#include <WinSock2.h>
#include <mswsock.h>

#include <iostream>
#include <thread>

#include "IocpCore.hpp"
#include "Listener.hpp"
#include "NetUtils.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
	if (!NetUtils::Init()) {
		return 1;
	}

	IocpCore iocp;
	if (!iocp.Init()) {
		NetUtils::PrintError("Failed to initialize IOCP core");
		WSACleanup();
		return 1;
	}

	Listener listener(&iocp, 8080);
	if (!listener.Init()) {
		NetUtils::PrintError("Failed to initialize listener");
		WSACleanup();
		return 1;
	}

	if (!iocp.Start(std::thread::hardware_concurrency())) {
		NetUtils::PrintError("Failed to start IOCP worker threads");
		WSACleanup();
		return 1;
	}

	if (!listener.PostAccept()) {
		NetUtils::PrintError("Failed to post initial accept");
		WSACleanup();
		return 1;
	}

	Sleep(INFINITE);

	WSACleanup();
}
