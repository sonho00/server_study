#include "IocpCore.hpp"

#include <WinSock2.h>

#include <memory>
#include <string>

#include "Listener.hpp"
#include "Network/Common/NetUtils.hpp"
#include "OverlappedEx.hpp"
#include "Session.hpp"

IocpCore::IocpCore() {
	hIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hIocp_ == nullptr) {
		throw std::runtime_error("Failed to create IOCP");
	}
}

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

bool IocpCore::Start(const size_t threadCount) {
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

bool IocpCore::Register(const SOCKET socket,
						const ULONG_PTR completionKey) const {
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

		OverlappedEx* pOverlappedEx =
			CONTAINING_RECORD(pOverlapped, OverlappedEx, overlapped_);

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

			if (pOverlappedEx->ioType_ == IO_TYPE::ACCEPT) {
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

		if (pOverlappedEx->ioType_ == IO_TYPE::ACCEPT) {
			Listener* listener = reinterpret_cast<Listener*>(completionKey);
			if (!listener->HandleAccept(pOverlappedEx)) {
				NetUtils::PrintError("Failed to handle accept");
				continue;
			}
		} else {
			std::shared_ptr<Session> objPtr =
				reinterpret_cast<Session*>(completionKey)->shared_from_this();

			if (bytesTransferred == 0) {
				std::string s = "Connection closed by client\n";
				std::cout << s;
				objPtr->Close();
				continue;
			}

			if (!objPtr->HandleIO(pOverlappedEx, bytesTransferred)) {
				NetUtils::PrintError("Failed to handle I/O operation");
				objPtr->Close();
				continue;
			}
		}
	}

	return 0;
}
