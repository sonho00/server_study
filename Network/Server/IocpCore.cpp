// NOLINTBEGIN(performance-no-int-to-ptr)
#include "IocpCore.hpp"

#include <WinSock2.h>
#include <winnt.h>

#include <thread>

#include "Listener.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "OverlappedEx.hpp"
#include "Session.hpp"

IocpCore::IocpCore() {
	hIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hIocp_ == nullptr) {
		LOG_FATAL("[Error:{}] Failed to create IOCP", GetLastError());
	}
}

IocpCore::~IocpCore() {
	for (auto& _ : threads_) {
		PostQueuedCompletionStatus(hIocp_, 0, 0, nullptr);
	}

	for (auto& thread : threads_) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	CloseHandle(hIocp_);
}

bool IocpCore::Start(size_t threadCount) {
	threads_.reserve(threadCount);
	for (size_t i = 0; i < threadCount; ++i) {
		threads_.emplace_back(&IocpCore::WorkerThread, this);
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

bool IocpCore::Register(SOCKET socket, ULONG_PTR completionKey) const {
	if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), hIocp_,
							   completionKey, 0) == nullptr) {
		LOG_ERROR("[Error:{}] Failed to register socket with IOCP",
				  GetLastError());
		return false;
	}
	return true;
}

void IocpCore::HandleError(OverlappedEx& overlappedEx) {
	SharedPoolPtr<Session> session = overlappedEx.sessionPtr_;
	DWORD errorCode = GetLastError();
	switch (errorCode) {
		case WSAENOTCONN:
			if (overlappedEx.ioType_ == IO_TYPE::kDisconnect) {
				LOG_ERROR(
					"[Session:{}][Error:{}] DisconnectEx failed - client "
					"already disconnected",
					session->GetHandle(), errorCode);
				session->Reset();
				return;
			}
		case ERROR_SUCCESS:
		case ERROR_IO_PENDING:
			LOG_INFO("[Session:{}][Error:{}] Graceful disconnect detected",
					 session->GetHandle(), errorCode);
			session->Disconnect();
			break;

		case ERROR_NETNAME_DELETED:
			LOG_INFO("[Session:{}][Error:{}] Abortive disconnect detected",
					 session->GetHandle(), errorCode);
			session->Disconnect();
			break;

		default:
			LOG_ERROR(
				"[Session:{}][Error:{}] I/O operation failed or connection "
				"closed unexpectedly ",
				session->GetHandle(), errorCode);
			session.Reset();
			break;
	}
}

void IocpCore::Dispatch(ULONG_PTR completionKey, OverlappedEx* overlappedEx,
						DWORD bytesTransferred) {
	SharedPoolPtr<Session> session = overlappedEx->sessionPtr_;
	switch (overlappedEx->ioType_) {
		case IO_TYPE::kAccept: {
			auto* listener = reinterpret_cast<Listener*>(completionKey);
			if (!listener->HandleAccept(session)) {
				LOG_ERROR("[Session:{}] Failed to handle accept",
						  session->GetHandle());
				session->Disconnect();
			}
			break;
		}
		case IO_TYPE::kDisconnect: {
			LOG_INFO("[Session:{}] Disconnect completed", session->GetHandle());
			session->Reset();
			break;
		}
		case IO_TYPE::kRecv:
		case IO_TYPE::kSend: {
			LOG_DEBUG("[Session:{}] Dispatching I/O event - IOType: {}",
					  session->GetHandle(),
					  static_cast<int>(overlappedEx->ioType_));

			if (bytesTransferred == 0) {
				LOG_INFO("[Session:{}] Connection closed by client",
						 session->GetHandle());
				session->Disconnect();
				return;
			}

			if (!session->HandleIO(*overlappedEx, bytesTransferred)) {
				LOG_ERROR("[Session:{}] Failed to handle I/O operation",
						  session->GetHandle());
				session->Disconnect();
			}
			break;
		}
		default:
			LOG_ERROR("[Session:{}] Unknown I/O type: {}",
					  overlappedEx->sessionPtr_->GetHandle(),
					  static_cast<int>(overlappedEx->ioType_));
			return;
	}
}

void IocpCore::WorkerThread() {
	while (true) {
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* overlapped = nullptr;

		BOOL result = GetQueuedCompletionStatus(
			hIocp_, &bytesTransferred, &completionKey, &overlapped, INFINITE);

		if (overlapped == nullptr) {
			if (result == TRUE) {
				LOG_INFO("Server is shutting down.");
				break;
			}
			LOG_FATAL("[Error:{}] GQCS failed", GetLastError());
		}

		OverlappedEx* overlappedEx =
			CONTAINING_RECORD(overlapped, OverlappedEx, overlapped_);

		if (result == FALSE) {
			HandleError(*overlappedEx);
			continue;
		}

		SharedPoolPtr<Session> session = overlappedEx->sessionPtr_;

		LOG_DEBUG("[Session:{}] IOType: {} BytesTransferred: {}",
				  session->GetHandle(), static_cast<int>(overlappedEx->ioType_),
				  bytesTransferred);

		Dispatch(completionKey, overlappedEx, bytesTransferred);
	}
}
// NOLINTEND(performance-no-int-to-ptr)
