#include "Listener.hpp"

#include <WinSock2.h>

#include <iostream>

#include "NetUtils.hpp"
#include "Session.hpp"

bool Listener::Init() {
	socket_ = NetUtils::CreateListenSocket(port_);
	if (socket_ == INVALID_SOCKET) {
		return false;
	}
	taskType_ = Task::ACCEPT;

	if (!iocpCore_->Register(socket_, (ULONG_PTR)this)) {
		NetUtils::PrintError("Failed to register listener socket with IOCP");
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
		return false;
	}

	return true;
}

bool Listener::Dispatch(OVERLAPPED* overlapped, DWORD bytesTransferred) {
	std::cout << "Client connected." << std::endl;
	PostAccept();

	Session* session = (Session*)((size_t)overlapped - 8);
	setsockopt(session->socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			   (char*)&socket_, sizeof(socket_));

	session->taskType_ = Task::READ;
	session->wsaBuf_.buf = session->buffer_;
	session->wsaBuf_.len = sizeof(session->buffer_);
	ZeroMemory(&session->overlapped_, sizeof(OVERLAPPED));

	DWORD flags = 0;
	int recvResult = WSARecv(session->socket_, &session->wsaBuf_, 1, NULL,
							 &flags, &session->overlapped_, NULL);

	if (recvResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSARecv failed");
		return false;
	}

	return true;
}

bool Listener::PostAccept() {
	SOCKET hAcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
									 WSA_FLAG_OVERLAPPED);
	if (hAcceptSocket == INVALID_SOCKET) {
		NetUtils::PrintError("Failed to create accept socket");
		return false;
	}

	Session* session = iocpCore_->sessionPool_.Pop();
	if (!session) {
		NetUtils::PrintError("Failed to get session from pool");
		iocpCore_->sessionPool_.Push(session);
		return false;
	}

	session->socket_ = hAcceptSocket;
	session->taskType_ = Task::ACCEPT;
	session->wsaBuf_.buf = session->buffer_;
	session->wsaBuf_.len = sizeof(session->buffer_);

	if (!iocpCore_->Register(hAcceptSocket, (ULONG_PTR)session)) {
		NetUtils::PrintError("Failed to register accept socket with IOCP");
		iocpCore_->sessionPool_.Push(session);
		return false;
	}

	DWORD bytesReceived = 0;
	DWORD addrLen = sizeof(sockaddr_in) + 16;
	BOOL result =
		NetUtils::AcceptEx(socket_, hAcceptSocket, session->buffer_, 0, addrLen,
						   addrLen, &bytesReceived, &session->overlapped_);

	if (!result && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("AcceptEx failed");
		iocpCore_->sessionPool_.Push(session);
		return false;
	}

	return true;
}
