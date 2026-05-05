#pragma once

#include <WinSock2.h>

#include <thread>
#include <vector>

#include "OverlappedEx.hpp"

class SessionManager;

struct IocpResult {
	BOOL result_;
	DWORD bytesTransferred_;
	ULONG_PTR completionKey_;
	OverlappedEx* overlappedEx_;
};

class IocpCore {
   public:
	IocpCore(SessionManager& sessionManager);
	~IocpCore();

	bool Start(size_t threadCount);
	[[nodiscard]] bool Register(SOCKET socket, ULONG_PTR completionKey) const;

   private:
	void WorkerThread();
	void LogIOEvent(const IocpResult& iocpResult);
	static void Dispatch(const IocpResult& iocpResult);
	bool HandleError(const IocpResult& iocpResult);

	std::vector<std::thread> threads_;
	HANDLE hIocp_;
	SessionManager& sessionManager_;
};
