#include "IocpCore.hpp"

#include <WinSock2.h>

#include "IocpObject.hpp"
#include "NetUtils.hpp"

IocpCore::~IocpCore() {
	for (HANDLE thread : threads_) {
		if (thread) {
			PostQueuedCompletionStatus(hIocp_, 0, 0, NULL);
		}
	}

	for (HANDLE thread : threads_) {
		if (thread) {
			WaitForSingleObject(thread, INFINITE);
			CloseHandle(thread);
		}
	}

	CloseHandle(hIocp_);
}

bool IocpCore::Init() {
	hIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hIocp_ == nullptr) {
		NetUtils::PrintError("Failed to create IOCP");
		return false;
	}

	return true;
}

bool IocpCore::Start(size_t threadCount) {
	threads_.resize(threadCount);
	for (size_t i = 0; i < threadCount; ++i) {
		threads_[i] = CreateThread(NULL, 0, WorkerThread, this, 0, NULL);
		if (threads_[i] == NULL) {
			NetUtils::PrintError("Failed to create worker thread");
			for (size_t j = 0; j < i; ++j) {
				CloseHandle(threads_[j]);
			}
			return false;
		}
	}

	return true;
}

bool IocpCore::Register(SOCKET socket, ULONG_PTR completionKey) {
	if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), hIocp_,
							   completionKey, 0) == nullptr) {
		NetUtils::PrintError("Failed to register socket with IOCP");
		return false;
	}
	return true;
}

DWORD WINAPI IocpCore::WorkerThread(LPVOID lpParam) {
	IocpCore* iocp = static_cast<IocpCore*>(lpParam);
	while (true) {
		IocpObject* iocpObject = nullptr;
		OVERLAPPED* pOverlapped = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;

		BOOL result =
			GetQueuedCompletionStatus(iocp->hIocp_, &bytesTransferred,
									  &completionKey, &pOverlapped, INFINITE);

		iocpObject = reinterpret_cast<IocpObject*>(completionKey);

		if (!result) {
			int error = WSAGetLastError();
			if (error == WSA_OPERATION_ABORTED) {
				std::cout << "IOCP is shutting down." << std::endl;
				break;
			}

			if (iocpObject) {
				if (iocpObject->taskType_ == Task::ACCEPT) {
					NetUtils::PrintError("Accept operation failed");
				} else {
					NetUtils::PrintError("I/O operation failed");
					iocp->sessionPool_.Push(static_cast<Session*>(iocpObject));
				}
				continue;
			}
		}

		if (bytesTransferred == 0 && iocpObject->taskType_ != Task::ACCEPT) {
			std::cout << "Connection closed by client." << std::endl;
			iocp->sessionPool_.Push(static_cast<Session*>(iocpObject));
			continue;
		}

		if (!iocpObject->Dispatch(pOverlapped, bytesTransferred)) {
			NetUtils::PrintError("Dispatch failed");
			iocp->sessionPool_.Push(static_cast<Session*>(iocpObject));
			continue;
		}
	}

	return 0;
}
