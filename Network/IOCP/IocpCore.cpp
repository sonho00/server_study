#include "IocpCore.hpp"

#include <WinSock2.h>
#include <basetsd.h>

#include <memory>

#include "Listener.hpp"
#include "NetUtils.hpp"
#include "OverlappedEx.hpp"
#include "Session.hpp"

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
		OVERLAPPED* pOverlapped = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;

		BOOL result =
			GetQueuedCompletionStatus(iocp->hIocp_, &bytesTransferred,
									  &completionKey, &pOverlapped, INFINITE);

		OverlappedEx<Session::kReadBufferSize>* pOverlappedEx =
			CONTAINING_RECORD(pOverlapped,
							  OverlappedEx<Session::kReadBufferSize>,
							  overlapped_);

		if (!pOverlappedEx) {
			if (result) {
				std::cout << "IOCP is shutting down." << std::endl;
				break;
			} else {
				NetUtils::PrintError("GetQueuedCompletionStatus failed");
				continue;
			}
		}

		if (!result) {
			int error = WSAGetLastError();
			if (error == WSA_OPERATION_ABORTED) {
				std::cout << "IOCP is shutting down." << std::endl;
				break;
			}

			if (pOverlappedEx->taskType_ == Task::ACCEPT) {
				NetUtils::PrintError("Accept operation failed");
			} else {
				NetUtils::PrintError("I/O operation failed");

				std::shared_ptr<Session> objPtr =
					reinterpret_cast<Session*>(completionKey)
						->shared_from_this();
				objPtr->Close();
			}
			continue;
		}

		if (pOverlappedEx->taskType_ == Task::ACCEPT) {
			Listener* listener = reinterpret_cast<Listener*>(completionKey);
			if (!listener->HandleAccept(pOverlappedEx, bytesTransferred)) {
				NetUtils::PrintError("Failed to handle accept");
				continue;
			}
		} else {
			std::shared_ptr<Session> objPtr =
				reinterpret_cast<Session*>(completionKey)->shared_from_this();

			if (bytesTransferred == 0) {
				std::cout << "Connection closed by client." << std::endl;
				objPtr->Close();
				continue;
			}

			if (!objPtr->HandleIO(pOverlappedEx, bytesTransferred)) {
				NetUtils::PrintError("Failed to handle I/O");
				objPtr->Close();
			}
		}
	}

	return 0;
}
