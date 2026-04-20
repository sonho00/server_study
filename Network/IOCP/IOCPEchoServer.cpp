#include <WinSock2.h>
#include <mswsock.h>

#include <iostream>
#include <thread>
#include <vector>

#include "ObjectPool.hpp"

#pragma comment(lib, "ws2_32.lib")

enum class Task { NONE, ACCEPT, READ, WRITE };

struct Session {
	OVERLAPPED overlapped = {};
	SOCKET socket = INVALID_SOCKET;
	WSABUF wsaBuf = {};
	char buffer[1024]{};
	Task taskType = Task::NONE;
};

HANDLE g_hIOCP = INVALID_HANDLE_VALUE;
SOCKET g_listenSocket = INVALID_SOCKET;
LPFN_ACCEPTEX g_lpfnAcceptEx = NULL;
ObjectPool<Session, 1024> g_sessionPool;

void PostAccept() {
	SOCKET hAcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
									 WSA_FLAG_OVERLAPPED);
	if (hAcceptSocket == INVALID_SOCKET) {
		std::cerr << "WSASocket for accept failed: " << WSAGetLastError()
				  << std::endl;
		return;
	}

	Session* session = g_sessionPool.Pop();
	if (!session) {
		std::cerr << "Failed to allocate session from pool." << std::endl;
		closesocket(hAcceptSocket);
		return;
	}
	session->socket = hAcceptSocket;
	session->taskType = Task::ACCEPT;
	session->wsaBuf.buf = session->buffer;
	session->wsaBuf.len = sizeof(session->buffer);
	CreateIoCompletionPort((HANDLE)hAcceptSocket, g_hIOCP, (ULONG_PTR)session,
						   0);

	DWORD bytesReceived = 0;
	DWORD addrLen = sizeof(sockaddr_in) + 16;
	BOOL result =
		g_lpfnAcceptEx(g_listenSocket, hAcceptSocket, session->buffer, 0,
					   addrLen, addrLen, &bytesReceived, &session->overlapped);

	if (!result && WSAGetLastError() != ERROR_IO_PENDING) {
		std::cerr << "AcceptEx failed: " << WSAGetLastError() << std::endl;
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
				std::cerr << "I/O operation failed: " << GetLastError()
						  << std::endl;
				closesocket(session->socket);
				g_sessionPool.Push(session);
				continue;
			} else {
				std::cerr << "GetQueuedCompletionStatus failed: "
						  << GetLastError() << std::endl;
				break;
			}
		}

		if (bytesTransferred == 0 && session->taskType != Task::ACCEPT) {
			std::cout << "Client disconnected." << std::endl;
			closesocket(session->socket);
			g_sessionPool.Push(session);
			continue;
		}

		switch (session->taskType) {
			case Task::ACCEPT: {
				std::cout << "Client connected." << std::endl;
				PostAccept();

				// AcceptEx로 새로 연결된 소켓을 리슨 소켓과 연결
				setsockopt(session->socket, SOL_SOCKET,
						   SO_UPDATE_ACCEPT_CONTEXT, (char*)&g_listenSocket,
						   sizeof(g_listenSocket));

				session->taskType = Task::READ;
				session->wsaBuf.buf = session->buffer;
				session->wsaBuf.len = sizeof(session->buffer);

				DWORD flags = 0;
				int recvResult =
					WSARecv(session->socket, &session->wsaBuf, 1, NULL, &flags,
							&session->overlapped, NULL);

				// WSARecv가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
				// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
				if (recvResult == SOCKET_ERROR &&
					WSAGetLastError() != ERROR_IO_PENDING) {
					std::cerr << "WSARecv failed: " << WSAGetLastError()
							  << std::endl;
					closesocket(session->socket);
					g_sessionPool.Push(session);
				}
				break;
			}
			case Task::READ: {
				std::cout << "Received message from client: "
						  << std::string(session->buffer, bytesTransferred)
						  << std::endl;

				session->taskType = Task::WRITE;
				session->wsaBuf.buf = session->buffer;
				session->wsaBuf.len = bytesTransferred;
				ZeroMemory(&session->overlapped, sizeof(OVERLAPPED));

				DWORD flags = 0;
				int sendResult =
					WSASend(session->socket, &session->wsaBuf, 1, NULL, flags,
							&session->overlapped, NULL);

				// WSASend가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
				// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
				if (sendResult == SOCKET_ERROR &&
					WSAGetLastError() != ERROR_IO_PENDING) {
					std::cerr << "WSASend failed: " << WSAGetLastError()
							  << std::endl;
					closesocket(session->socket);
					g_sessionPool.Push(session);
				}
				break;
			}
			case Task::WRITE: {
				session->taskType = Task::READ;
				session->wsaBuf.buf = session->buffer;
				session->wsaBuf.len = sizeof(session->buffer);
				ZeroMemory(&session->overlapped, sizeof(OVERLAPPED));

				DWORD flags = 0;
				int recvResult =
					WSARecv(session->socket, &session->wsaBuf, 1, NULL, &flags,
							&session->overlapped, NULL);

				// WSARecv가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
				// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
				if (recvResult == SOCKET_ERROR &&
					WSAGetLastError() != ERROR_IO_PENDING) {
					std::cerr << "WSARecv failed: " << WSAGetLastError()
							  << std::endl;
					closesocket(session->socket);
					g_sessionPool.Push(session);
				}
				break;
			}
		}
	}
}

int main() {
	std::cout << "Starting echo server..." << std::endl;

	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
		return 1;
	}

	std::cout << "Echo server started on port 8080..." << std::endl;

	g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (g_hIOCP == NULL) {
		std::cerr << "CreateIoCompletionPort failed: " << GetLastError()
				  << std::endl;
		return 1;
	}

	std::cout << "IOCP created successfully." << std::endl;

	g_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
							   WSA_FLAG_OVERLAPPED);
	if (g_listenSocket == INVALID_SOCKET) {
		std::cerr << "WSASocket failed: " << WSAGetLastError() << std::endl;
		CloseHandle(g_hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Socket created successfully." << std::endl;

	if (CreateIoCompletionPort((HANDLE)g_listenSocket, g_hIOCP, 0, 0) == NULL) {
		std::cerr << "CreateIoCompletionPort for listen socket failed: "
				  << GetLastError() << std::endl;
		closesocket(g_listenSocket);
		CloseHandle(g_hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Listen socket associated with IOCP." << std::endl;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(8080);
	if (bind(g_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
		closesocket(g_listenSocket);
		CloseHandle(g_hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Socket bound to port 8080." << std::endl;

	if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
		closesocket(g_listenSocket);
		CloseHandle(g_hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Listening for incoming connections..." << std::endl;

	// AcceptEx 함수 포인터 로드
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	if (WSAIoctl(g_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
				 &guidAcceptEx, sizeof(guidAcceptEx), &g_lpfnAcceptEx,
				 sizeof(g_lpfnAcceptEx), &dwBytes, NULL,
				 NULL) == SOCKET_ERROR) {
		std::cerr << "WSAIoctl failed: " << WSAGetLastError() << std::endl;
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
			std::cerr << "Failed to create worker thread: " << GetLastError()
					  << std::endl;
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

	std::cout << "Echo server is running. Press Enter to stop..." << std::endl;

	Sleep(INFINITE);

	CloseHandle(g_hIOCP);
	closesocket(g_listenSocket);
	WSACleanup();

	std::cout << "Echo server stopped." << std::endl;
}
