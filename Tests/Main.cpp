#include <Winsock2.h>

#include <array>
#include <functional>
#include <memory>

#include "Client.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/WSAManager.hpp"
#include "TestCase/01_Fragmentation.hpp"
#include "TestCase/02_StickyPackets.hpp"

std::array<char, 1 << 20> Client::sendBuf_{};
std::array<char, 1 << 20> Client::recvBuf_{};

int main() {
	WSAManager wsaManager;

	LOG_INFO("Test client started.");

	std::vector<std::function<std::unique_ptr<Client>()>> testFactories = {
		[&]() { return std::make_unique<Fragmentation>(); },
		[&]() { return std::make_unique<StickyPackets>(); },
	};

	for (int i = 0; i < testFactories.size(); ++i) {
		for (int j = 0; j < 10; ++j) {
			auto test = testFactories[i]();
			if (!test->test()) {
				LOG_ERROR("Test failed on iteration {}.", j);
				return 1;
			}
		}
		LOG_INFO("Test {} passed.", i + 1);
	}

	return 0;
}
