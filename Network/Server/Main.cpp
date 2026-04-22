#include <WinSock2.h>
#include <basetsd.h>
#include <mswsock.h>

#include <iostream>
#include <thread>

#include "IocpCore.hpp"
#include "Listener.hpp"
#include "NetUtils.hpp"
#include "WSAManager.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
	std::cout << "Starting echo server..." << std::endl;

	WSAManager wsaManager;	// WSAStartup과 WSACleanup을 RAII 방식으로 관리

	std::cout << "WSAStartup successful." << std::endl;

	if (!NetUtils::Init()) {
		return 1;
	}

	std::cout << "Network utilities initialized successfully." << std::endl;

	IocpCore iocp;
	if (!iocp.Init()) {
		NetUtils::PrintError("Failed to initialize IOCP core");
		return 1;
	}

	std::cout << "IOCP core initialized successfully." << std::endl;

	Listener listener(&iocp, static_cast<u_short>(8080));
	if (!listener.Init()) {
		NetUtils::PrintError("Failed to initialize listener");
		return 1;
	}

	std::cout << "Listener initialized successfully." << std::endl;

	if (!iocp.Start(std::thread::hardware_concurrency())) {
		NetUtils::PrintError("Failed to start IOCP worker threads");
		return 1;
	}

	std::cout << "IOCP worker threads started successfully." << std::endl;

	if (!listener.PostAccept()) {
		NetUtils::PrintError("Failed to post initial accept");
		return 1;
	}

	std::cout << "Initial accept posted successfully. Server is ready to "
				 "accept connections."
			  << std::endl;

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	std::cout << "Echo server is running. Press Enter to stop..." << std::endl;
	std::cin.get();

	std::cout << "Shutting down server..." << std::endl;

	return 0;
}
