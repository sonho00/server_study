#include <WinSock2.h>
#include <mswsock.h>

#include <iostream>
#include <thread>
#include <vector>

#include "NetUtils.hpp"
#include "ObjectPool.hpp"
#include "Session.hpp"

#pragma comment(lib, "ws2_32.lib")

HANDLE g_hIOCP = INVALID_HANDLE_VALUE;
SOCKET g_listenSocket = INVALID_SOCKET;
LPFN_ACCEPTEX g_lpfnAcceptEx = NULL;
ObjectPool<Session, 1024> g_sessionPool;

void PostAccept() {
	SOCKET hAcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
									 WSA_FLAG_OVERLAPPED);
	if (hAcceptSocket == INVALID_SOCKET) {
		NetUtils::PrintError("WSASocket for accept failed");
		return;
	}

	Session* session = g_sessionPool.Pop();
	if (!session) {
		NetUtils::PrintError("No available session objects");
		closesocket(hAcceptSocket);
		return;
	}
	session->socket_ = hAcceptSocket;
	session->taskType_ = Task::ACCEPT;
	session->wsaBuf_.buf = session->buffer_;
	session->wsaBuf_.len = sizeof(session->buffer_);
	CreateIoCompletionPort((HANDLE)hAcceptSocket, g_hIOCP, (ULONG_PTR)session,
						   0);

	DWORD bytesReceived = 0;
	DWORD addrLen = sizeof(sockaddr_in) + 16;
	BOOL result =
		g_lpfnAcceptEx(g_listenSocket, hAcceptSocket, session->buffer_, 0,
					   addrLen, addrLen, &bytesReceived, &session->overlapped_);

	if (!result && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("AcceptEx failed");
		closesocket(hAcceptSocket);
		g_sessionPool.Push(session);
	}
}

void WorkerThread() {
	while (true) {
		Session* session = nullptr;
		OVERLAPPED* pOverlapped = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;

		BOOL result = GetQueuedCompletionStatus(
			g_hIOCP, &bytesTransferred, &completionKey, &pOverlapped, INFINITE);

		// 리슨 소켓에서의 ACCEPT 작업은 completionKey가 0이고
		// 다른 작업들은 completionKey가 Session 포인터를 가리킴
		if (completionKey == 0)
			session = (Session*)pOverlapped;
		else
			session = (Session*)completionKey;

		if (!result) {
			if (session) {
				NetUtils::PrintError("I/O operation failed");
				closesocket(session->socket_);
				g_sessionPool.Push(session);
				continue;
			} else {
				NetUtils::PrintError("GetQueuedCompletionStatus failed");
				break;
			}
		}

		if (bytesTransferred == 0 && session->taskType_ != Task::ACCEPT) {
			std::cout << "Client disconnected." << std::endl;
			closesocket(session->socket_);
			g_sessionPool.Push(session);
			continue;
		}

		switch (session->taskType_) {
			case Task::ACCEPT: {
				std::cout << "Client connected." << std::endl;
				PostAccept();

				// AcceptEx로 새로 연결된 소켓을 리슨 소켓과 연결
				setsockopt(session->socket_, SOL_SOCKET,
						   SO_UPDATE_ACCEPT_CONTEXT, (char*)&g_listenSocket,
						   sizeof(g_listenSocket));

				session->taskType_ = Task::READ;
				session->wsaBuf_.buf = session->buffer_;
				session->wsaBuf_.len = sizeof(session->buffer_);

				DWORD flags = 0;
				int recvResult =
					WSARecv(session->socket_, &session->wsaBuf_, 1, NULL,
							&flags, &session->overlapped_, NULL);

				// WSARecv가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
				// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
				if (recvResult == SOCKET_ERROR &&
					WSAGetLastError() != ERROR_IO_PENDING) {
					NetUtils::PrintError("WSARecv failed");
					closesocket(session->socket_);
					g_sessionPool.Push(session);
				}
				break;
			}

			case Task::READ: {
				if (!session->OnRead(bytesTransferred)) {
					closesocket(session->socket_);
					g_sessionPool.Push(session);
				}
				break;
			}

			case Task::WRITE: {
				if (!session->OnWrite()) {
					closesocket(session->socket_);
					g_sessionPool.Push(session);
				}
				break;
			}

			default: {
				std::cerr << "Unknown task type." << std::endl;
				closesocket(session->socket_);
				g_sessionPool.Push(session);
				break;
			}
		}
	}
}

int main() {
	std::cout << "Starting echo server..." << std::endl;

	if (!NetUtils::Init()) {
		return 1;
	}

	std::cout << "Echo server started on port 8080..." << std::endl;

	g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (g_hIOCP == NULL) {
		NetUtils::PrintError("CreateIoCompletionPort failed");
		return 1;
	}

	std::cout << "IOCP created successfully." << std::endl;

	g_listenSocket = NetUtils::CreateListenSocket(8080);
	if (g_listenSocket == INVALID_SOCKET) {
		NetUtils::PrintError("Failed to create listen socket");
		CloseHandle(g_hIOCP);
		WSACleanup();
		return 1;
	}
	std::cout << "Socket created successfully." << std::endl;

	if (CreateIoCompletionPort((HANDLE)g_listenSocket, g_hIOCP, 0, 0) == NULL) {
		NetUtils::PrintError("CreateIoCompletionPort for listen socket failed");
		closesocket(g_listenSocket);
		CloseHandle(g_hIOCP);
		WSACleanup();
		return 1;
	}

	g_lpfnAcceptEx = NetUtils::GetAcceptEx(g_listenSocket);
	if (!g_lpfnAcceptEx) {
		NetUtils::PrintError("Failed to get AcceptEx function pointer");
		closesocket(g_listenSocket);
		CloseHandle(g_hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "AcceptEx function pointer loaded." << std::endl;

	const int numThreads = std::thread::hardware_concurrency();
	std::vector<HANDLE> threads(numThreads);
	for (int i = 0; i < numThreads; ++i) {
		threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkerThread,
								  NULL, 0, NULL);
		if (threads[i] == NULL) {
			NetUtils::PrintError("Failed to create worker thread");
			for (int j = 0; j < i; ++j) {
				CloseHandle(threads[j]);
			}
			closesocket(g_listenSocket);
			CloseHandle(g_hIOCP);
			WSACleanup();
			return 1;
		}
	}

	std::cout << "Worker threads created: " << numThreads << std::endl;

	PostAccept();

	Sleep(INFINITE);

	CloseHandle(g_hIOCP);
	closesocket(g_listenSocket);
	WSACleanup();

	std::cout << "Echo server stopped." << std::endl;
}
