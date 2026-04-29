#include "IocpCore.hpp"

#include <WinSock2.h>
#include <ioapiset.h>

#include <memory>
#include <thread>

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
	for (std::thread& thread : threads_) {
		PostQueuedCompletionStatus(hIocp_, 0, 0, nullptr);
	}

	for (std::thread& thread : threads_) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	CloseHandle(hIocp_);
}

bool IocpCore::Start(const size_t threadCount) {
	threads_.resize(threadCount);
	for (size_t i = 0; i < threadCount; ++i) {
		threads_[i] = std::thread(&IocpCore::WorkerThread, this);
		if (!threads_[i].joinable()) {
			LOG_FATAL("Failed to create worker thread");
			for (size_t j = 0; j < i; ++j) {
				PostQueuedCompletionStatus(hIocp_, 0, 0, nullptr);
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

void IocpCore::WorkerThread() {
	while (true) {
		OVERLAPPED* ov = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;

		BOOL result = GetQueuedCompletionStatus(hIocp_, &bytesTransferred,
												&completionKey, &ov, INFINITE);

		if (!ov) {
			if (result) {
				LOG_INFO("IOCP is shutting down.");
				break;
			} else {
				LOG_ERROR("GetQueuedCompletionStatus failed: {}",
						  GetLastError());
				continue;
			}
		}

		OverlappedEx& ovEx = *CONTAINING_RECORD(ov, OverlappedEx, overlapped_);
		if (ovEx.ioType_ == IO_TYPE::ACCEPT) {
			Session* session = CONTAINING_RECORD(&ovEx, Session, readOv);
			LOG_DEBUG("[Session:{}] IOType: {} BytesTransferred: {}",
					  session->GetHandle(), static_cast<int>(ovEx.ioType_),
					  bytesTransferred);
		} else {
			Session* session = reinterpret_cast<Session*>(completionKey);
			LOG_DEBUG("[Session:{}] IOType: {} BytesTransferred: {}",
					  session->GetHandle(), static_cast<int>(ovEx.ioType_),
					  bytesTransferred);
		}

		if (!result) {
			int error = WSAGetLastError();
			if (error == WSA_OPERATION_ABORTED) {
				LOG_INFO("IOCP is shutting down.");
				break;
			}

			if (ovEx.ioType_ == IO_TYPE::ACCEPT) {
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

		if (ovEx.ioType_ == IO_TYPE::ACCEPT) {
			Listener* listener = reinterpret_cast<Listener*>(completionKey);
			if (!listener->HandleAccept(ovEx)) {
				LOG_ERROR("Failed to handle accept");
			}
		} else {
			std::shared_ptr<Session> objPtr =
				reinterpret_cast<Session*>(completionKey)->shared_from_this();

			if (bytesTransferred == 0) {
				LOG_INFO("Connection closed by client");
				objPtr->Close();
			}

			if (!objPtr->HandleIO(ovEx, bytesTransferred)) {
				LOG_ERROR("Failed to handle I/O operation");
				objPtr->Close();
			}
		}
	}
}
