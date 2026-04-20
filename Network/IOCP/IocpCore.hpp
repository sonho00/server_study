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

	bool Init(SOCKET listenSocket);
	bool Start(size_t threadCount);
	bool Register(SOCKET socket, ULONG_PTR completionKey);
	bool PostAccept();

   private:
	static DWORD WINAPI WorkerThread(LPVOID lpParam);

	HANDLE hIocp_;
	SOCKET listenSocket_;

	std::vector<HANDLE> threads_;
	ObjectPool<Session, 1024> sessionPool_;
};
