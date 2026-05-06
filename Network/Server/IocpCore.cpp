// NOLINTBEGIN(performance-no-int-to-ptr)
#include "IocpCore.hpp"

#include <WinSock2.h>

#include <thread>

#include "Listener.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/SharedPoolPtr.hpp"
#include "Session.hpp"
#include "SessionManager.hpp"

IocpCore::IocpCore(SessionManager& sessionManager)
	: sessionManager_(sessionManager) {
	hIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hIocp_ == nullptr) {
		LOG_FATAL("Failed to create IOCP: {}", GetLastError());
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
		LOG_ERROR("Failed to register socket with IOCP: {}", GetLastError());
		return false;
	}
	return true;
}

void IocpCore::HandleAccept(const IocpResult& iocpResult) {
	if (iocpResult.result_ == FALSE) {
		DWORD errorCode = GetLastError();
		switch (errorCode) {
			case ERROR_OPERATION_ABORTED:
				LOG_INFO(
					"Accept operation was aborted, likely due to server "
					"shutdown.");
				return;

			default:
				LOG_ERROR("Accept operation failed with error code: {}",
						  errorCode);
				break;
		}

		auto* session =
			CONTAINING_RECORD(iocpResult.overlappedEx_, Session, readOv_);
		LOG_DEBUG(
			"[Listener] Failed to accept connection - Session Handle: "
			"{}",
			session->GetHandle());
	} else {
		Session* session =
			CONTAINING_RECORD(iocpResult.overlappedEx_, Session, readOv_);
		LOG_INFO("[Listener] Accepted new connection - Session Handle: {}",
				 session->GetHandle());

		auto* listener = reinterpret_cast<Listener*>(iocpResult.completionKey_);
		if (!listener->HandleAccept(
				static_cast<const OverlappedEx&>(*iocpResult.overlappedEx_))) {
			LOG_ERROR("Failed to handle accept");
			session->Close();
		}
	}
}

void IocpCore::HandleError(const IocpResult& iocpResult) {
	SharedPoolPtr<Session> session =
		sessionManager_.GetSession(iocpResult.completionKey_);
	DWORD errorCode = GetLastError();
	switch (errorCode) {
		case ERROR_SUCCESS:
		case ERROR_IO_PENDING:
			LOG_INFO("[Session:{}] Graceful disconnect detected",
					 session->GetHandle());
			break;

		case ERROR_NETNAME_DELETED:
			LOG_INFO("[Session:{}] Abortive disconnect detected",
					 session->GetHandle());
			break;

		default:
			LOG_ERROR(
				"[Session:{}] I/O operation failed or connection "
				"closed: "
				"{}",
				session->GetHandle(), errorCode);
			break;
	}
	session->Close();
}

void IocpCore::LogIOEvent(const IocpResult& iocpResult) {
	SharedPoolPtr<Session> session =
		sessionManager_.GetSession(iocpResult.completionKey_);

	LOG_DEBUG("[Session:{}] IOType: {} BytesTransferred: {}",
			  session->GetHandle(),
			  static_cast<int>(iocpResult.overlappedEx_->ioType_),
			  iocpResult.bytesTransferred_);
}

void IocpCore::Dispatch(const IocpResult& iocpResult) {
	SharedPoolPtr<Session> session =
		sessionManager_.GetSession(iocpResult.completionKey_);

	LOG_DEBUG("[Session:{}] Dispatching I/O event - IOType: {}",
			  session->GetHandle(),
			  static_cast<int>(iocpResult.overlappedEx_->ioType_));

	if (!session->HandleIO(*iocpResult.overlappedEx_,
						   iocpResult.bytesTransferred_)) {
		LOG_ERROR("[Session:{}] Failed to handle I/O operation",
				  session->GetHandle());
		session->Close();
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
			LOG_FATAL("GQCS failed: {}", GetLastError());
		}

		OverlappedEx* overlappedEx =
			CONTAINING_RECORD(overlapped, OverlappedEx, overlapped_);
		IocpResult iocpResult = {.result_ = result,
								 .bytesTransferred_ = bytesTransferred,
								 .completionKey_ = completionKey,
								 .overlappedEx_ = overlappedEx};

		if (overlappedEx->ioType_ == IO_TYPE::kAccept) {
			HandleAccept(iocpResult);
			continue;
		}

		SharedPoolPtr<Session> session =
			sessionManager_.GetSession(completionKey);

		if (result == 0 || bytesTransferred == 0) {
			HandleError(iocpResult);
			continue;
		}

		LogIOEvent(iocpResult);
		Dispatch(iocpResult);
	}
}
// NOLINTEND(performance-no-int-to-ptr)
