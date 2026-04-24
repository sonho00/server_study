#pragma once

#include <thread>

#include "IocpCore.hpp"
#include "Listener.hpp"
#include "Network/Common/NetUtils.hpp"
#include "Network/Common/WSAManager.hpp"
#include "ServerUtils.hpp"

class ServerService {
   public:
	ServerService()
		: iocpCore_(std::make_unique<IocpCore>()),
		  listener_(iocpCore_.get(), 8080, netFuncs_.AcceptEx) {}

	void Start() {
		int numThreads = std::thread::hardware_concurrency();
		iocpCore_->Start(numThreads);

		for (int i = 0; i < 2000; ++i) {
			if (!listener_.PostAccept()) {
				NetUtils::PrintError("Failed to post initial accept");
			}
		}
	}

   private:
	WSAManager wsaManager_;
	ServerUtils::NetFuncs netFuncs_;
	std::unique_ptr<IocpCore> iocpCore_;
	Listener listener_;
};
