#include "Listener.hpp"

#include <WinSock2.h>

#include <iostream>

#include "IocpCore.hpp"
#include "Network/Common/NetUtils.hpp"
#include "OverlappedEx.hpp"
#include "ServerUtils.hpp"
#include "Session.hpp"

Listener::Listener(IocpCore* iocpCore, const uint16_t port,
				   LPFN_ACCEPTEX acceptEx)
	: iocpCore_(iocpCore), port_(port), acceptEx_(acceptEx) {
	socket_ = ServerUtils::CreateListenSocket(port_);
	if (socket_ == INVALID_SOCKET) {
		throw std::runtime_error("Failed to create listen socket");
	}

	acceptOv.ioType_ = IO_TYPE::ACCEPT;
	acceptOv.wsaBuf_.buf = acceptOv.buffer_.GetBuffer();
	acceptOv.wsaBuf_.len = static_cast<ULONG>(acceptOv.buffer_.GetSize()) - 1;

	if (!iocpCore_->Register(socket_, reinterpret_cast<ULONG_PTR>(this))) {
		throw std::runtime_error(
			"Failed to register listener socket with IOCP");
	}
}

Listener::~Listener() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

bool Listener::HandleAccept(const OverlappedEx* overlappedEx) {
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

	if (!session->RegisterRead()) {
		NetUtils::PrintError("Failed to post initial read");
		session->Close();
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

	std::shared_ptr<Session> session = iocpCore_->sessionPool_.Acquire();
	if (!session) {
		NetUtils::PrintError("Failed to get session from pool");
		return false;
	}

	session->readOv.ioType_ = IO_TYPE::ACCEPT;
	session->readOv.wsaBuf_.buf = session->readOv.buffer_.GetBuffer();
	session->readOv.wsaBuf_.len =
		static_cast<ULONG>(session->readOv.buffer_.GetSize());
	session->socket_ = hAcceptSocket;

	if (!iocpCore_->Register(hAcceptSocket,
							 reinterpret_cast<ULONG_PTR>(session.get()))) {
		NetUtils::PrintError("Failed to register accept socket with IOCP");
		session->Close();
		return false;
	}

	DWORD bytesReceived = 0;
	DWORD addrLen = sizeof(sockaddr_in) + 16;
	BOOL result = acceptEx_(
		socket_, hAcceptSocket, session->readOv.buffer_.GetBuffer(), 0, addrLen,
		addrLen, &bytesReceived, &session->readOv.overlapped_);

	if (!result && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("AcceptEx failed");
		session->Close();
		return false;
	}

	return true;
}
