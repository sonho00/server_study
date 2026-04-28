#pragma once

#include <thread>

#include "IocpCore.hpp"
#include "Listener.hpp"
#include "Network/Common/NetUtils.hpp"
#include "Network/Common/WSAManager.hpp"
#include "Network/Server/SessionManager.hpp"
#include "ServerUtils.hpp"

class ServerService {
   public:
	ServerService()
		: listener_(iocpCore_, sessionManager_, 8080, netFuncs_.AcceptEx) {}

	bool Start() {
		int numThreads = std::thread::hardware_concurrency();
		if (!iocpCore_.Start(numThreads)) {
			LOG_FATAL("Failed to start IOCP worker threads");
		}

		for (int i = 0; i < 16; ++i) {
			if (!listener_.PostAccept()) {
				LOG_ERROR("Failed to post initial accept");
				return false;
			}
		}

		return true;
	}

   private:
	WSAManager wsaManager_;
	ServerUtils::NetFuncs netFuncs_;
	IocpCore iocpCore_;
	SessionManager sessionManager_;
	Listener listener_;
};
