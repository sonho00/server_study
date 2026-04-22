#include "Listener.hpp"

#include <WinSock2.h>

#include <iostream>

#include "NetUtils.hpp"
#include "OverlappedEx.hpp"
#include "Session.hpp"

bool Listener::Init() {
	socket_ = NetUtils::CreateListenSocket(port_);
	if (socket_ == INVALID_SOCKET) {
		NetUtils::PrintError("Failed to create listen socket");
		return false;
	}

	acceptOv.taskType_ = Task::ACCEPT;
	acceptOv.wsaBuf_.buf = acceptOv.buffer_.GetBuffer();
	acceptOv.wsaBuf_.len = static_cast<ULONG>(acceptOv.buffer_.GetSize()) - 1;

	if (!iocpCore_->Register(socket_, reinterpret_cast<ULONG_PTR>(this))) {
		NetUtils::PrintError("Failed to register listener socket with IOCP");
		return false;
	}

	return true;
}

bool Listener::HandleAccept(OverlappedEx* overlappedEx,
							DWORD bytesTransferred) {
	std::cout << "Client connected." << std::endl;
	if (!PostAccept()) {
		NetUtils::PrintError("Failed to post accept");
		return false;
	}

	Session* session = CONTAINING_RECORD(overlappedEx, Session, readOv);
	if (setsockopt(session->socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				   reinterpret_cast<const char*>(&socket_),
				   sizeof(socket_)) == SOCKET_ERROR) {
		NetUtils::PrintError("setsockopt failed");
		return false;
	}

	session->readOv.taskType_ = Task::RECV;
	session->readOv.wsaBuf_.buf = session->readOv.buffer_.GetBuffer();
	session->readOv.wsaBuf_.len =
		static_cast<ULONG>(session->readOv.buffer_.GetSize()) - 1;
	ZeroMemory(&session->readOv.overlapped_, sizeof(OVERLAPPED));

	DWORD flags = 0;
	int recvResult = WSARecv(session->socket_, &session->readOv.wsaBuf_, 1,
							 NULL, &flags, &session->readOv.overlapped_, NULL);

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

	std::shared_ptr<Session> session = iocpCore_->sessionPool_.Pop();
	if (!session) {
		NetUtils::PrintError("Failed to get session from pool");
		session->Close();
		return false;
	}

	session->readOv.taskType_ = Task::ACCEPT;
	session->readOv.wsaBuf_.buf = session->readOv.buffer_.GetBuffer();
	session->readOv.wsaBuf_.len =
		static_cast<ULONG>(session->readOv.buffer_.GetSize()) - 1;
	session->socket_ = hAcceptSocket;

	if (!iocpCore_->Register(hAcceptSocket,
							 reinterpret_cast<ULONG_PTR>(session.get()))) {
		NetUtils::PrintError("Failed to register accept socket with IOCP");
		session->Close();
		return false;
	}

	DWORD bytesReceived = 0;
	DWORD addrLen = sizeof(sockaddr_in) + 16;
	BOOL result = NetUtils::AcceptEx(
		socket_, hAcceptSocket, session->readOv.buffer_.GetBuffer(), 0, addrLen,
		addrLen, &bytesReceived, &session->readOv.overlapped_);

	if (!result && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("AcceptEx failed");
		session->Close();
		return false;
	}

	return true;
}
