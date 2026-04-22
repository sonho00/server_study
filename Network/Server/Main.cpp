#include <iostream>

#include "Network/Server/ServerService.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
	ServerService serverService;
	serverService.Start();

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	std::cout << "Echo server is running. Press Enter to stop..." << std::endl;
	std::cin.get();

	std::cout << "Shutting down server..." << std::endl;

	return 0;
}
