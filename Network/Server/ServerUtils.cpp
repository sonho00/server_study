#include "ServerUtils.hpp"

#include <WinSock2.h>

#include "Network/Common/Logger.hpp"
#include "Session.hpp"

namespace ServerUtils {
NetFuncs::NetFuncs() {
	SOCKET dummySocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr,
								   0, WSA_FLAG_OVERLAPPED);
	if (dummySocket == INVALID_SOCKET) {
		LOG_FATAL("[Error:{}] Failed to create dummy socket",
				  WSAGetLastError());
	}

	DWORD bytesReturned = 0;

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	int result =
		WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
				 sizeof(guidAcceptEx), static_cast<void*>(&AcceptEx),
				 sizeof(AcceptEx), &bytesReturned, nullptr, nullptr);

	if (result == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] WSAIoctl failed to get AcceptEx pointer",
				  WSAGetLastError());
	}

	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	result = WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
					  &guidDisconnectEx, sizeof(guidDisconnectEx),
					  static_cast<void*>(&DisconnectEx), sizeof(DisconnectEx),
					  &bytesReturned, nullptr, nullptr);

	if (result == SOCKET_ERROR) {
		LOG_FATAL("[Error:{}] WSAIoctl failed to get DisconnectEx pointer",
				  WSAGetLastError());
	}

	closesocket(dummySocket);
}

void HandleError(SharedPoolPtr<Session>& session, int errorCode) {
	switch (errorCode) {
		case ERROR_SUCCESS:
		case WSAECONNRESET:
			LOG_INFO("[Session:{}][Error:{}] Graceful disconnect detected",
					 session->GetHandle(), errorCode);
			session->Disconnect();
			break;

		case ERROR_NETNAME_DELETED:
			LOG_INFO("[Session:{}][Error:{}] Abortive disconnect detected",
					 session->GetHandle(), errorCode);
			session->Disconnect();
			break;

		case ERROR_OPERATION_ABORTED:
			LOG_WARN(
				"[Session:{}][Error:{}] Operation aborted, likely due to "
				"server shutdown",
				session->GetHandle(), errorCode);
			session->Disconnect();
			break;

		case ERROR_IO_PENDING:
			LOG_INFO(
				"[Session:{}][Error:{}] I/O operation pending, no immediate "
				"error",
				session->GetHandle(), errorCode);
			break;

		case WSAEINVAL:
			LOG_WARN(
				"[Session:{}][Error:{}] Invalid argument, possible programming "
				"error or resource exhaustion",
				session->GetHandle(), errorCode);
			break;

		default:
			LOG_ERROR(
				"[Session:{}][Error:{}] I/O operation failed or connection "
				"closed unexpectedly ",
				session->GetHandle(), errorCode);
			session->Disconnect();
			break;
	}
}
LPFN_ACCEPTEX AcceptEx = nullptr;
LPFN_DISCONNECTEX DisconnectEx = nullptr;
}  // namespace ServerUtils
