#include <WinSock2.h>

#include <cstring>
#include <iostream>
#include <thread>
#include <vector>


#pragma comment(lib, "ws2_32.lib")

enum class IO_TYPE { READ, WRITE };

struct OverlappedEx {
	OVERLAPPED overlapped;
	SOCKET socket;
	WSABUF wsaBuf;
	char buffer[1024];
	IO_TYPE ioType;
};

DWORD WINAPI WorkerThread(LPVOID lpParam) {
	HANDLE hIOCP = (HANDLE)lpParam;

	while (true) {
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;
		OverlappedEx* ov = nullptr;

		BOOL result =
			GetQueuedCompletionStatus(hIOCP, &bytesTransferred, &completionKey,
									  (LPOVERLAPPED*)&ov, INFINITE);

		if (!result) {
			if (ov) {
				std::cerr << "I/O operation failed: " << GetLastError()
						  << std::endl;
				closesocket(ov->socket);
				delete ov;
				continue;
			} else {
				std::cerr << "GetQueuedCompletionStatus failed: "
						  << GetLastError() << std::endl;
				break;
			}
		}

		if (bytesTransferred == 0) {
			std::cout << "Client disconnected." << std::endl;
			if (ov) {
				closesocket(ov->socket);
				delete ov;
			}
			continue;
		}

		if (ov->ioType == IO_TYPE::READ) {
			ov->ioType = IO_TYPE::WRITE;
			ov->wsaBuf.len = bytesTransferred;
			memset(&ov->overlapped, 0, sizeof(OVERLAPPED));

			DWORD sentBytes = 0;
			if (WSASend(ov->socket, &ov->wsaBuf, 1, &sentBytes, 0,
						&ov->overlapped, NULL) == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					std::cerr << "WSASend failed: " << WSAGetLastError()
							  << std::endl;
					closesocket(ov->socket);
					delete ov;
				}
			}
		} else if (ov->ioType == IO_TYPE::WRITE) {
			ov->ioType = IO_TYPE::READ;
			ov->wsaBuf.len = 1024;
			memset(ov->buffer, 0, 1024);
			memset(&ov->overlapped, 0, sizeof(OVERLAPPED));

			DWORD flags = 0;
			DWORD recvBytes = 0;
			if (WSARecv(ov->socket, &ov->wsaBuf, 1, &recvBytes, &flags,
						&ov->overlapped, NULL) == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					std::cerr << "WSARecv failed: " << WSAGetLastError()
							  << std::endl;
					closesocket(ov->socket);
					delete ov;
				}
			}
		}
	}

	return 0;
}

int main() {
	std::cout << "Starting IOCP Echo Server..." << std::endl;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
		return 1;
	}

	std::cout << "WSAStartup successful." << std::endl;

	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hIOCP == NULL) {
		std::cerr << "CreateIoCompletionPort failed: " << GetLastError()
				  << std::endl;
		return 1;
	}

	std::cout << "IOCP created successfully." << std::endl;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
		CloseHandle(hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Socket created successfully." << std::endl;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(8080);
	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		CloseHandle(hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Socket bound to port 8080." << std::endl;

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		CloseHandle(hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Listening on port 8080." << std::endl;

	if (CreateIoCompletionPort((HANDLE)listenSocket, hIOCP,
							   (ULONG_PTR)listenSocket, 0) == NULL) {
		std::cerr << "Failed to associate listen socket with IOCP: "
				  << GetLastError() << std::endl;
		closesocket(listenSocket);
		CloseHandle(hIOCP);
		WSACleanup();
		return 1;
	}

	std::cout << "Listen socket associated with IOCP." << std::endl;

	const int numThreads = std::thread::hardware_concurrency();
	std::vector<HANDLE> threads(numThreads);
	for (int i = 0; i < numThreads; ++i) {
		threads[i] = CreateThread(NULL, 0, WorkerThread, hIOCP, 0, NULL);
		if (threads[i] == NULL) {
			std::cerr << "Failed to create worker thread: " << GetLastError()
					  << std::endl;
			for (int j = 0; j < i; ++j) {
				CloseHandle(threads[j]);
			}
			closesocket(listenSocket);
			CloseHandle(hIOCP);
			WSACleanup();
			return 1;
		}
	}

	std::cout<< "Worker threads created: " << numThreads << std::endl;

	while (true) {
		sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);

		SOCKET clientSocket =
			accept(listenSocket, (sockaddr*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) continue;

		std::cout << "Client connected!" << std::endl;

		CreateIoCompletionPort((HANDLE)clientSocket, hIOCP,
							   (ULONG_PTR)clientSocket, 0);

		OverlappedEx* ov = new OverlappedEx();
		memset(ov, 0, sizeof(OverlappedEx));
		ov->socket = clientSocket;
		ov->ioType = IO_TYPE::READ;
		ov->wsaBuf.buf = ov->buffer;
		ov->wsaBuf.len = 1024;

		DWORD flags = 0;
		DWORD recvBytes = 0;

		if (WSARecv(clientSocket, &ov->wsaBuf, 1, &recvBytes, &flags,
					&ov->overlapped, NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				std::cerr << "WSARecv failed: " << WSAGetLastError()
						  << std::endl;
				closesocket(clientSocket);
				delete ov;
			}
		}
	}

	CloseHandle(hIOCP);
	WSACleanup();

	std::cout << "IOCP Echo Server shutting down." << std::endl;
}
