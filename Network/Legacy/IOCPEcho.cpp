#include <WinSock2.h>
#include <mswsock.h>

#include <iostream>
#include <thread>

#include "Network/Common/NetUtils.hpp"
#include "Network/Common/WSAManager.hpp"
#include "Network/Server/IocpCore.hpp"
#include "Network/Server/Listener.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
	WSAManager wsaManager;
	IocpCore iocp;

	Listener listener(&iocp, 8080);

	if (!iocp.Start(std::thread::hardware_concurrency())) {
		LOG_FATAL("Failed to start IOCP worker threads");
	}

	if (!listener.PostAccept()) {
		LOG_FATAL("Failed to post initial accept");
	}

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	LOG_INFO("Press Enter to shut down the server...");
	std::cin.get();
	LOG_INFO("Server is shutting down...");

	return 0;
}
