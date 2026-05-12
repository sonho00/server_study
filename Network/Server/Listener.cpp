#include "Listener.hpp"

#include <WinSock2.h>

#include <cstdint>

#include "IocpCore.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "OverlappedEx.hpp"
#include "ServerUtils.hpp"
#include "SessionManager.hpp"

Listener::Listener(IocpCore& iocpCore, SessionManager& sessionManager,
				   uint16_t port)
	: iocpCore_(iocpCore), sessionManager_(sessionManager), port_(port) {
	socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
						WSA_FLAG_OVERLAPPED);
	if (socket_ == INVALID_SOCKET) {
		LOG_FATAL("[Error:{}] WSASocket failed", WSAGetLastError());
	}

	acceptOv_.ioType_ = IO_TYPE::kAccept;
	acceptOv_.wsaBuf_.buf = acceptOv_.buffer_.GetBuffer();
	acceptOv_.wsaBuf_.len = static_cast<ULONG>(acceptOv_.buffer_.GetSize()) - 1;
	ZeroMemory(&acceptOv_.overlapped_, sizeof(OVERLAPPED));

	int opt = 1;
	if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
				   sizeof(opt)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(SO_REUSEADDR) failed: {}", WSAGetLastError());
	}

	if (!iocpCore_.Register(socket_, reinterpret_cast<ULONG_PTR>(this))) {
		LOG_FATAL("Failed to register listener socket with IOCP");
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(socket_, reinterpret_cast<sockaddr*>(&serverAddr),
			 sizeof(serverAddr)) == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] bind failed", WSAGetLastError());
	}

	if (listen(socket_, SOMAXCONN) == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] listen failed", WSAGetLastError());
	}
}

Listener::~Listener() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

bool Listener::HandleAccept(SharedPoolPtr<Session>& session) {
	pendingAccepts_--;
	PostAccept();

	if (setsockopt(session->GetSocket(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				   reinterpret_cast<const char*>(&socket_),
				   sizeof(socket_)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed: {}",
				  WSAGetLastError());
		return false;
	}

	int opt = 1;
	if (setsockopt(session->GetSocket(), IPPROTO_TCP, TCP_NODELAY,
				   reinterpret_cast<const char*>(&opt),
				   sizeof(opt)) == SOCKET_ERROR) {
		LOG_ERROR("setsockopt(TCP_NODELAY) failed: {}", WSAGetLastError());
		return false;
	}

	if (!session->RegisterRead()) {
		LOG_ERROR("Failed to post initial read");
		return false;
	}

	return true;
}

bool Listener::PostAccept() {
	while (pendingAccepts_ < Config::kAcceptCount) {
		SharedPoolPtr<Session> session = sessionManager_.CreateSession();
		if (!session.IsValid()) return false;

		if (!RegisterAccept(session)) {
			LOG_ERROR("Failed to post AcceptEx");
			session->Disconnect();
			return false;
		}

		session->listener_ = this;
		pendingAccepts_++;
		LOG_DEBUG("Posted AcceptEx - Pending accepts: {}", pendingAccepts_);
	}
	return true;
}

// NOLINTNEXTLINE readability-make-member-function-const
bool Listener::RegisterAccept(SharedPoolPtr<Session>& session) {
	session->readOv_.ioType_ = IO_TYPE::kAccept;
	session->readOv_.wsaBuf_.buf = session->readOv_.buffer_.GetBuffer();
	session->readOv_.wsaBuf_.len =
		static_cast<ULONG>(session->readOv_.buffer_.GetSize());
	ZeroMemory(&session->readOv_.overlapped_, sizeof(OVERLAPPED));
	session->readOv_.sessionPtr_ = session;

	DWORD bytesReceived = 0;
	BOOL result = ServerUtils::AcceptEx(
		socket_, session->GetSocket(), session->readOv_.buffer_.GetBuffer(), 0,
		Config::kAcceptAddrSize, Config::kAcceptAddrSize, &bytesReceived,
		&session->readOv_.overlapped_);

	if (result == 0) {
		int errorCode = WSAGetLastError();
		if (errorCode != ERROR_IO_PENDING) {
			LOG_ERROR("AcceptEx failed: {}", errorCode);
			return false;
		}
	}

	return true;
}
