#pragma once

#include <thread>

#include "Network/IOCP/IocpCore.hpp"
#include "Network/IOCP/Listener.hpp"
#include "Network/IOCP/NetUtils.hpp"
#include "Network/IOCP/ObjectPool.hpp"
#include "Network/IOCP/Session.hpp"
#include "Network/IOCP/WSAManager.hpp"

class ServerService {
   public:
	ServerService()
		: iocpCore_(std::make_unique<IocpCore>()),
		  listener_(iocpCore_.get(), 8080, netFuncs_.AcceptEx) {}

	void Start() {
		int numThreads = std::thread::hardware_concurrency();
		iocpCore_->Start(numThreads);
		for (int i = 0; i < numThreads; ++i) {
			if (!listener_.PostAccept()) {
				NetUtils::PrintError("Failed to post initial accept");
			}
		}
	}

   private:
	WSAManager wsaManager_;
	NetUtils::NetFuncs netFuncs_;
	std::unique_ptr<IocpCore> iocpCore_;
	ObjectPool<Session> sessionManager_;
	Listener listener_;
};
