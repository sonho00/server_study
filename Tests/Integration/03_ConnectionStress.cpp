#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "Network/Common/Logger.hpp"
#include "Tests/Base/Client.hpp"

class ConnectionStress : public Client {
   public:
	void ThreadFunc() override {
		// 1000개의 클라이언트가 접속, 종료
		for (int i = 0; i < 1000; ++i) {
			ConnectionStress client;
			for (int j = 0; j < 32; ++j) {
				client.CreateSocket();
				client.DefaultSockOpt();
				client.AdditionalSockOpt();

				int result = client.Connect();
				switch (result) {
					case 0:
						break;	// 성공

					case WSAECONNREFUSED:
						LOG_WARN("Connection refused, retrying...");
						Sleep(1ul << j);
						continue;

					default:
						LOG_ERROR("Failed to connect: {}", WSAGetLastError());
						return;
				}

				if (result == 0) break;
			}
			LOG_DEBUG("Client {} connected", i + 1);
		}
	}

	bool test() override {
		std::vector<std::thread> clientThreads;
		clientThreads.reserve(100);
		for (int i = 0; i < 100; ++i) {
			clientThreads.emplace_back(&ConnectionStress::ThreadFunc, this);
			LOG_DEBUG("Started client thread {}", i + 1);
		}
		int cnt = 0;
		for (auto& thread : clientThreads) {
			thread.join();
			++cnt;
			if (cnt % 100 == 0) LOG_INFO("Joined {} threads", cnt);
		}
		// 테스트 끝날 때까지 서버가 다운되지 않으면 성공
		return true;
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
