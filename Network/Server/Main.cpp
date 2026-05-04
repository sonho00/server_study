#include <iostream>

#include "ServerService.hpp"

int main() {
	auto serverService = std::make_unique<ServerService>();
	serverService->Start();

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	LOG_INFO("Server is running. Press Enter to shut down...");
	std::cin.get();
	LOG_INFO("Server is shutting down...");

	return 0;
}
