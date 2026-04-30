#include <Winsock2.h>

#include <thread>

#include "01.hpp"
#include "Network/Common/WSAManager.hpp"

int main() {
	WSAManager wsaManager;

	LOG_INFO("Test client started.");

	const char* serverIp = "127.0.0.1";
	uint16_t serverPort = 8080;
	Fragmentation fragmentation(serverIp, serverPort);
	std::thread clientThread(&Fragmentation::ThreadFunc, &fragmentation);

	// 유저의 종료 신호를 기다립니다. 예: Enter 키
	LOG_INFO("Press Enter to stop the client...");
	std::cin.get();
	clientThread.join();
	LOG_INFO("Test client stopped.");

	return 0;
}
