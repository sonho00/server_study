#pragma once

#include "IocpCore.hpp"
#include "IocpObject.hpp"

class Listener : public IocpObject {
   public:
	Listener(IocpCore* iocpCore, unsigned short port)
		: iocpCore_(iocpCore), port_(port) {}

	bool Init();
	bool Dispatch(OVERLAPPED* overlapped, DWORD bytesTransferred) override;
	bool PostAccept();

   private:
	char buffer_[sizeof(sockaddr_in) + 16] = {};
	IocpCore* iocpCore_ = nullptr;
	unsigned short port_ = 0;
};
