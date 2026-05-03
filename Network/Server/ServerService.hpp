#pragma once

#include <thread>

#include "IocpCore.hpp"
#include "Listener.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/WSAManager.hpp"
#include "Network/Server/SessionManager.hpp"
#include "ServerUtils.hpp"

class ServerService {
   public:
	ServerService()
		: iocpCore_(sessionManager_),
		  listener_(iocpCore_, sessionManager_, Config::kPort,
					netFuncs_.acceptEx_) {}

	bool Start() {
		size_t numThreads = std::thread::hardware_concurrency();
		if (!iocpCore_.Start(numThreads)) {
			LOG_FATAL("Failed to start IOCP worker threads");
		}

		for (size_t i = 0; i < Config::kInitialAcceptCount; ++i) {
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
	SessionManager sessionManager_;
	IocpCore iocpCore_;
	Listener listener_;
};
