#pragma once

#include <WinSock2.h>

#include <thread>
#include <vector>

#include "OverlappedEx.hpp"

class IocpCore {
   public:
	IocpCore();
	~IocpCore();

	bool Start(size_t threadCount);
	[[nodiscard]] bool Register(SOCKET socket, ULONG_PTR completionKey) const;

   private:
	void WorkerThread();

	static void HandleError(OverlappedEx& overlappedEx);
	static void Dispatch(ULONG_PTR completionKey, OverlappedEx* overlappedEx,
						 DWORD bytesTransferred);

	std::vector<std::thread> threads_;
	HANDLE hIocp_;
};
