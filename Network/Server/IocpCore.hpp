#pragma once

#include <WinSock2.h>

#include <thread>
#include <vector>

#include "OverlappedEx.hpp"

struct IocpResult {
	BOOL result_;
	DWORD bytesTransferred_;
	ULONG_PTR completionKey_;
	OverlappedEx* overlappedEx_;
};

class IocpCore {
   public:
	IocpCore();
	~IocpCore();

	bool Start(size_t threadCount);
	[[nodiscard]] bool Register(SOCKET socket, ULONG_PTR completionKey) const;

   private:
	void WorkerThread();
	static void LogIOEvent(const IocpResult& iocpResult);
	static void Dispatch(const IocpResult& iocpResult);
	static bool HandleError(const IocpResult& iocpResult);

	std::vector<std::thread> threads_;
	HANDLE hIocp_;
};
