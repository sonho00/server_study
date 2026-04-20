#include "Session.hpp"

#include "NetUtils.hpp"

bool Session::OnRead(DWORD bytesTransferred) {
	std::cout << "Received message from client: "
			  << std::string(buffer_, bytesTransferred) << std::endl;

	taskType_ = Task::WRITE;
	wsaBuf_.buf = buffer_;
	wsaBuf_.len = bytesTransferred;
	ZeroMemory(&overlapped_, sizeof(OVERLAPPED));

	DWORD flags = 0;
	int sendResult =
		WSASend(socket_, &wsaBuf_, 1, NULL, flags, &overlapped_, NULL);

	// WSASend가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
	// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
	if (sendResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSASend failed");
		return false;
	}

	return true;
}

bool Session::OnWrite() {
	std::cout << "Sent message to client: " << std::string(buffer_, wsaBuf_.len)
			  << std::endl;

	taskType_ = Task::READ;
	wsaBuf_.buf = buffer_;
	wsaBuf_.len = sizeof(buffer_);
	ZeroMemory(&overlapped_, sizeof(OVERLAPPED));

	DWORD flags = 0;
	int recvResult =
		WSARecv(socket_, &wsaBuf_, 1, NULL, &flags, &overlapped_, NULL);

	// WSARecv가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
	// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
	if (recvResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSARecv failed");
		return false;
	}

	return true;
}
