#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "Network/Common/Logger.hpp"
#include "Tests/Client.hpp"

class ConnectionStress : public Client {
   public:
	void ThreadFunc() override {
		// 1000개의 클라이언트가 접속, 종료
		try {
			for (int i = 0; i < 1000; ++i) {
				ConnectionStress client;
				while (true) {
					client.CreateSocket();
					client.DefaultSockOpt();
					client.AdditionalSockOpt();

					int result = client.Connect();
					switch (result) {
						case 0:
							break;	// 성공

						case WSAECONNREFUSED:
							LOG_WARN("Connection refused, retrying...");
							Sleep(100);
							continue;

						default:
							LOG_ERROR("Failed to connect: {}",
									  WSAGetLastError());
							return;
					}

					if (result == 0) break;
				}
				LOG_DEBUG("Client {} connected", i + 1);
			}
		} catch (const std::exception& e) {
			LOG_ERROR("Exception in client thread: {}", e.what());
			success_ = false;
		}
	}

	bool test() override {
		try {
			std::vector<std::thread> clientThreads;
			clientThreads.reserve(1000);
			for (int i = 0; i < 1000; ++i) {
				clientThreads.emplace_back(&ConnectionStress::ThreadFunc, this);
				LOG_DEBUG("Started client thread {}", i + 1);
			}
			int cnt = 0;
			for (auto& thread : clientThreads) {
				thread.join();
				++cnt;
				if (cnt % 100 == 0) LOG_INFO("Joined {} threads", cnt);
			}
		} catch (const std::exception& e) {
			LOG_ERROR("Exception in ConnectionStress test: {}", e.what());
			success_ = false;
		}
		return success_;
	}

	void AdditionalSockOpt() override {
		struct linger solinger;
		solinger.l_onoff = 1;	// Linger 옵션 활성화
		solinger.l_linger = 0;	// 대기 시간 0 (즉시 강제 종료, RST 패킷 전송)

		if (setsockopt(socket_, SOL_SOCKET, SO_LINGER, (char*)&solinger,
					   sizeof(solinger)) == SOCKET_ERROR) {
			LOG_ERROR("setsockopt SO_LINGER failed: {}", WSAGetLastError());
		}
	}
};

TEST(NetworkTest, ConnectionStress) {
	ConnectionStress client;
	EXPECT_TRUE(client.test());
}
