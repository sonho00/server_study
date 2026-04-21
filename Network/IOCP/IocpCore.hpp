#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <vector>

#include "ObjectPool.hpp"
#include "Session.hpp"

class IocpCore {
   public:
	IocpCore() = default;
	~IocpCore();

	bool Init();
	bool Start(size_t threadCount);
	bool Register(SOCKET socket, ULONG_PTR completionKey);

	ObjectPool<Session, 16> sessionPool_;

   private:
	static DWORD WINAPI WorkerThread(LPVOID lpParam);

	HANDLE hIocp_;

	std::vector<HANDLE> threads_;
};
