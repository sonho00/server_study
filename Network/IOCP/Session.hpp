#pragma once

#include <WinSock2.h>

#include "IocpObject.hpp"

class Session : public IocpObject {
   public:
	bool OnRead(DWORD bytesTransferred);
	bool OnWrite();
	bool Dispatch(OVERLAPPED* overlapped, DWORD bytesTransferred) override;

	char buffer_[1024]{};
};
