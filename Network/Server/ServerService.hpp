#pragma once

#include <thread>

#include "IocpCore.hpp"
#include "Listener.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/WSAManager.hpp"
#include "ServerUtils.hpp"
#include "SessionManager.hpp"

class ServerService {
   public:
	ServerService() : listener_(iocpCore_, sessionManager_, Config::kPort) {}

	bool Start() {
		iocpCore_.SetListener(&listener_);
		sessionManager_.Init(iocpCore_);

		size_t numThreads = std::thread::hardware_concurrency();
		if (!iocpCore_.Start(numThreads)) {
			LOG_FATAL("Failed to start IOCP worker threads");
		}

		if (!listener_.PostAccept()) {
			LOG_ERROR("Failed to post initial accept");
			return false;
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
