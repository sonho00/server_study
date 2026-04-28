#pragma once

#include <WinSock2.h>

#include <thread>
#include <vector>

class IocpCore {
   public:
	IocpCore();
	~IocpCore();

	bool Start(const size_t threadCount);
	bool Register(const SOCKET socket, const ULONG_PTR completionKey) const;

   private:
	void WorkerThread();

	std::vector<std::thread> threads_;
	HANDLE hIocp_;
};
