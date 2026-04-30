// NOLINTBEGIN(performance-no-int-to-ptr)
#include "IocpCore.hpp"

#include <WinSock2.h>
#include <ioapiset.h>

#include <memory>
#include <thread>

#include "Listener.hpp"
#include "Network/Common/NetUtils.hpp"
#include "Session.hpp"

IocpCore::IocpCore() {
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

bool IocpCore::HandleError(const IocpResult& iocpResult) {
	if (iocpResult.overlappedEx_ == nullptr) {
		if (iocpResult.result_ == FALSE) {
			LOG_INFO("IOCP is shutting down.");
			return true;
		}

		LOG_ERROR("GetQueuedCompletionStatus failed: {}", GetLastError());
		return false;
	}

	// result = false
	int error = WSAGetLastError();
	if (error == WSA_OPERATION_ABORTED) {
		LOG_INFO("IOCP is shutting down.");
		return true;
	}

	if (iocpResult.overlappedEx_->ioType_ == IO_TYPE::kAccept) {
		LOG_ERROR("Accept operation failed: {}", error);
	} else {
		LOG_ERROR("I/O operation failed: {}", error);

		std::shared_ptr<Session> objPtr =
			reinterpret_cast<Session*>(iocpResult.completionKey_)
				->shared_from_this();
		objPtr->Close();
	}
	return false;
}

void IocpCore::LogIOEvent(const IocpResult& iocpResult) {
	if (iocpResult.overlappedEx_->ioType_ == IO_TYPE::kAccept) {
		Session* session =
			CONTAINING_RECORD(iocpResult.overlappedEx_, Session, readOv_);
		LOG_DEBUG("[Session:{}] IOType: {} BytesTransferred: {}",
				  session->GetHandle(),
				  static_cast<int>(iocpResult.overlappedEx_->ioType_),
				  iocpResult.bytesTransferred_);
	} else {
		auto* session = reinterpret_cast<Session*>(iocpResult.completionKey_);
		LOG_DEBUG("[Session:{}] IOType: {} BytesTransferred: {}",
				  session->GetHandle(),
				  static_cast<int>(iocpResult.overlappedEx_->ioType_),
				  iocpResult.bytesTransferred_);
	}
}

void IocpCore::Dispatch(const IocpResult& iocpResult) {
	if (iocpResult.overlappedEx_->ioType_ == IO_TYPE::kAccept) {
		auto* listener = reinterpret_cast<Listener*>(iocpResult.completionKey_);
		if (!listener->HandleAccept(
				static_cast<const OverlappedEx&>(*iocpResult.overlappedEx_))) {
			LOG_ERROR("Failed to handle accept");
		}
	} else {
		std::shared_ptr<Session> objPtr =
			reinterpret_cast<Session*>(iocpResult.completionKey_)
				->shared_from_this();

		if (iocpResult.bytesTransferred_ == 0) {
			LOG_INFO("Connection closed by client");
			objPtr->Close();
		}

		if (!objPtr->HandleIO(*iocpResult.overlappedEx_,
							  iocpResult.bytesTransferred_)) {
			LOG_ERROR("Failed to handle I/O operation");
			objPtr->Close();
		}
	}
}

void IocpCore::WorkerThread() {
	while (true) {
		OVERLAPPED* overlapped = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;

		BOOL result = GetQueuedCompletionStatus(
			hIocp_, &bytesTransferred, &completionKey, &overlapped, INFINITE);

		OverlappedEx* overlappedEx =
			CONTAINING_RECORD(overlapped, OverlappedEx, overlapped_);

		IocpResult iocpResult{.result_ = result,
							  .bytesTransferred_ = bytesTransferred,
							  .completionKey_ = completionKey,
							  .overlappedEx_ = overlappedEx};

		if (result == FALSE || overlappedEx == nullptr) {
			if (HandleError(iocpResult)) {
				break;
			}
			continue;
		}

		LogIOEvent(iocpResult);

		Dispatch(iocpResult);
	}
}
// NOLINTEND(performance-no-int-to-ptr)
