#include <Winsock2.h>

#include <array>
#include <memory>

#include "Client.hpp"
#include "Network/Common/Config.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/WSAManager.hpp"
#include "TestCase/01_Fragmentation.hpp"
#include "TestCase/02_StickyPackets.hpp"

std::array<char, 1 << 20> Client::sendBuf_{};
std::array<char, 1 << 20> Client::recvBuf_{};

int main() {
	WSAManager wsaManager;

	LOG_INFO("Test client started.");

	const char* serverIp = "127.0.0.1";
	uint16_t serverPort = Config::kPort;

	std::unique_ptr<Client> tests[] = {
		std::unique_ptr<Fragmentation>(new Fragmentation(serverIp, serverPort)),
		std::unique_ptr<StickyPackets>(
			new StickyPackets(serverIp, serverPort))};

	for (int i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
		auto& test = tests[i];
		for (int j = 0; j < 10000; ++j) {
			if (!test->test()) {
				LOG_ERROR("Test failed on iteration {}.", j);
				return 1;
			}
			test->success_ = true;
			test->testBytes_ = 0;
			memset(test->sendBuf_.data(), 0, test->sendBuf_.size());
			memset(test->recvBuf_.data(), 0, test->recvBuf_.size());
		}
		LOG_INFO("Test {} passed.", i + 1);
	}

	return 0;
}
