#pragma once

#include <WinSock2.h>

#include <vector>

#include "Network/Common/ObjectPool.hpp"
#include "Session.hpp"

class IocpCore {
   public:
	IocpCore();
	~IocpCore();

	bool Start(const size_t threadCount);
	bool Register(const SOCKET socket, const ULONG_PTR completionKey) const;

	ObjectPool<Session, 4096> sessionPool_;

   private:
	static DWORD WINAPI WorkerThread(LPVOID lpParam);

	std::vector<HANDLE> threads_;
	HANDLE hIocp_;
};
