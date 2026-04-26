#include "IocpCore.hpp"

#include <WinSock2.h>

#include <memory>

#include "Listener.hpp"
#include "Network/Common/NetUtils.hpp"
#include "OverlappedEx.hpp"
#include "Session.hpp"

IocpCore::IocpCore() {
	hIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hIocp_ == nullptr) {
		LOG_FATAL("Failed to create IOCP: {}", GetLastError());
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
			LOG_FATAL("Failed to create worker thread: {}", GetLastError());
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
		LOG_ERROR("Failed to register socket with IOCP: {}", GetLastError());
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
				LOG_INFO("IOCP is shutting down.");
				break;
			} else {
				LOG_ERROR("GetQueuedCompletionStatus failed: {}",
						  GetLastError());
				continue;
			}
		}

		LOG_DEBUG(
			"GQCS - IOType: {} CompletionKey: {:#x} Overlapped: {} "
			"BytesTransferred: {}",
			static_cast<int>(pOverlappedEx->ioType_), completionKey,
			static_cast<void*>(pOverlappedEx), bytesTransferred);

		if (!result) {
			int error = WSAGetLastError();
			if (error == WSA_OPERATION_ABORTED) {
				LOG_INFO("IOCP is shutting down.");
				break;
			}

			if (pOverlappedEx->ioType_ == IO_TYPE::ACCEPT) {
				LOG_ERROR("Accept operation failed: {}", error);
			} else {
				LOG_ERROR("I/O operation failed: {}", error);

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
				LOG_ERROR("Failed to handle accept");
			}
		} else {
			std::shared_ptr<Session> objPtr =
				reinterpret_cast<Session*>(completionKey)->shared_from_this();

			if (bytesTransferred == 0) {
				LOG_INFO("Connection closed by client");
				objPtr->Close();
			}

			if (!objPtr->HandleIO(pOverlappedEx, bytesTransferred)) {
				LOG_ERROR("Failed to handle I/O operation");
				objPtr->Close();
			}
		}
	}

	return 0;
}
