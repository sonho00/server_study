#include "IocpCore.hpp"

#include <WinSock2.h>

#include "NetUtils.hpp"

IocpCore::~IocpCore() {
	for (HANDLE thread : threads_) {
		if (thread) {
			WaitForSingleObject(thread, INFINITE);
			CloseHandle(thread);
		}
	}

	CloseHandle(hIocp_);
	closesocket(listenSocket_);
}

bool IocpCore::Init(SOCKET listenSocket) {
	hIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hIocp_ == nullptr) {
		NetUtils::PrintError("Failed to create IOCP");
		return false;
	}

	listenSocket_ = listenSocket;
	if (!Register(listenSocket_, 0)) {
		NetUtils::PrintError("Failed to register listen socket with IOCP");
		CloseHandle(hIocp_);
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
	if (CreateIoCompletionPort((HANDLE)socket, hIocp_, completionKey, 0) ==
		nullptr) {
		NetUtils::PrintError("Failed to register socket with IOCP");
		return false;
	}
	return true;
}

bool IocpCore::PostAccept() {
	SOCKET hAcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
									 WSA_FLAG_OVERLAPPED);
	if (hAcceptSocket == INVALID_SOCKET) {
		NetUtils::PrintError("WSASocket for accept failed");
		return false;
	}

	Session* session = sessionPool_.Pop();
	if (!session) {
		NetUtils::PrintError("No available session objects");
		closesocket(hAcceptSocket);
		return false;
	}

	session->socket_ = hAcceptSocket;
	session->taskType_ = Task::ACCEPT;
	session->wsaBuf_.buf = session->buffer_;
	session->wsaBuf_.len = sizeof(session->buffer_);

	if (!Register(hAcceptSocket, (ULONG_PTR)session)) {
		closesocket(hAcceptSocket);
		sessionPool_.Push(session);
		return false;
	}

	DWORD bytesReceived = 0;
	DWORD addrLen = sizeof(sockaddr_in) + 16;
	BOOL result = NetUtils::AcceptEx(listenSocket_, hAcceptSocket,
									 session->buffer_, 0, addrLen, addrLen,
									 &bytesReceived, &session->overlapped_);

	if (!result && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("AcceptEx failed");
		closesocket(hAcceptSocket);
		sessionPool_.Push(session);
		return false;
	}

	return true;
}

DWORD WINAPI IocpCore::WorkerThread(LPVOID lpParam) {
	IocpCore* iocp = (IocpCore*)lpParam;
	while (true) {
		Session* session = nullptr;
		OVERLAPPED* pOverlapped = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;

		BOOL result =
			GetQueuedCompletionStatus(iocp->hIocp_, &bytesTransferred,
									  &completionKey, &pOverlapped, INFINITE);
		if (completionKey == 0)
			session = (Session*)pOverlapped;
		else
			session = (Session*)completionKey;

		if (!result) {
			if (session) {
				NetUtils::PrintError("I/O operation failed");
				closesocket(session->socket_);
				iocp->sessionPool_.Push(session);
				continue;
			} else {
				NetUtils::PrintError("GetQueuedCompletionStatus failed");
				break;
			}
		}

		if (bytesTransferred == 0 && session->taskType_ != Task::ACCEPT) {
			std::cout << "Client disconnected." << std::endl;
			closesocket(session->socket_);
			iocp->sessionPool_.Push(session);
			continue;
		}

		switch (session->taskType_) {
			case Task::ACCEPT: {
				std::cout << "Client connected." << std::endl;
				iocp->PostAccept();

				setsockopt(
					session->socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
					(char*)&iocp->listenSocket_, sizeof(iocp->listenSocket_));

				session->taskType_ = Task::READ;
				session->wsaBuf_.buf = session->buffer_;
				session->wsaBuf_.len = sizeof(session->buffer_);

				DWORD flags = 0;
				int recvResult =
					WSARecv(session->socket_, &session->wsaBuf_, 1, NULL,
							&flags, &session->overlapped_, NULL);

				if (recvResult == SOCKET_ERROR &&
					WSAGetLastError() != ERROR_IO_PENDING) {
					NetUtils::PrintError("WSARecv failed");
					closesocket(session->socket_);
					iocp->sessionPool_.Push(session);
				}
				break;
			}

			case Task::READ: {
				if (!session->OnRead(bytesTransferred)) {
					closesocket(session->socket_);
					iocp->sessionPool_.Push(session);
				}
				break;
			}

			case Task::WRITE: {
				if (!session->OnWrite()) {
					closesocket(session->socket_);
					iocp->sessionPool_.Push(session);
				}
				break;
			}

			default:
				std::cerr << "Unknown task type." << std::endl;
				closesocket(session->socket_);
				iocp->sessionPool_.Push(session);
				break;
		}
	}

	return 0;
}
