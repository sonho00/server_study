#include "Listener.hpp"

#include <WinSock2.h>

#include "IocpCore.hpp"
#include "Network/Common/NetUtils.hpp"
#include "OverlappedEx.hpp"
#include "ServerUtils.hpp"
#include "SessionManager.hpp"

Listener::Listener(IocpCore& iocpCore, SessionManager& sessionManager,
				   const uint16_t port, LPFN_ACCEPTEX acceptEx)
	: iocpCore_(iocpCore),
	  sessionManager_(sessionManager),
	  port_(port),
	  acceptEx_(acceptEx) {
	socket_ = ServerUtils::CreateListenSocket(port_);
	if (socket_ == INVALID_SOCKET) {
		LOG_FATAL("Failed to create listen socket");
	}

	int opt = 1;
	if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
				   sizeof(opt)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(SO_REUSEADDR) failed: {}", WSAGetLastError());
	}

	int tcp_opt = 1;
	if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char*)&tcp_opt,
				   sizeof(tcp_opt)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(TCP_NODELAY) failed: {}", WSAGetLastError());
	}

	acceptOv.ioType_ = IO_TYPE::ACCEPT;
	acceptOv.wsaBuf_.buf = acceptOv.buffer_.GetBuffer();
	acceptOv.wsaBuf_.len = static_cast<ULONG>(acceptOv.buffer_.GetSize()) - 1;

	if (!iocpCore_.Register(socket_, reinterpret_cast<ULONG_PTR>(this))) {
		LOG_FATAL("Failed to register listener socket with IOCP");
	}
}

Listener::~Listener() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

bool Listener::HandleAccept(const OverlappedEx& ovEx) {
	PostAccept();

	Session& session = *CONTAINING_RECORD(&ovEx, Session, readOv);
	if (setsockopt(session.socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				   reinterpret_cast<const char*>(&socket_),
				   sizeof(socket_)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed: {}",
				  WSAGetLastError());
		return false;
	}

	sessionManager_.AddSession(session);
	session.readOv.owner_ = nullptr;

	if (!session.RegisterRead()) {
		LOG_ERROR("Failed to post initial read");
		session.Close();
		return false;
	}

	return true;
}

bool Listener::PostAccept() {
	SOCKET hAcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
									 WSA_FLAG_OVERLAPPED);
	if (hAcceptSocket == INVALID_SOCKET) {
		LOG_ERROR("Failed to create accept socket");
		return false;
	}

	std::shared_ptr<Session> session = sessionManager_.CreateSession();
	if (!session) return false;

	session->readOv.owner_ = session;

	session->readOv.ioType_ = IO_TYPE::ACCEPT;
	session->readOv.wsaBuf_.buf = session->readOv.buffer_.GetBuffer();
	session->readOv.wsaBuf_.len =
		static_cast<ULONG>(session->readOv.buffer_.GetSize());
	session->socket_ = hAcceptSocket;

	if (!iocpCore_.Register(hAcceptSocket,
							reinterpret_cast<ULONG_PTR>(session.get()))) {
		LOG_ERROR("Failed to register accept socket with IOCP");
		session->Close();
		return false;
	}

	DWORD bytesReceived = 0;
	DWORD addrLen = sizeof(sockaddr_in) + 16;
	BOOL result = acceptEx_(
		socket_, hAcceptSocket, session->readOv.buffer_.GetBuffer(), 0, addrLen,
		addrLen, &bytesReceived, &session->readOv.overlapped_);

	if (!result) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("AcceptEx failed: {}", errorCode);
			session->Close();
			return false;
		}
	}

	return true;
}
