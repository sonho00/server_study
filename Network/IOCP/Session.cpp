#include "Session.hpp"

#include <WinSock2.h>
#include <minwinbase.h>
#include <minwindef.h>

#include "NetUtils.hpp"
#include "OverlappedEx.hpp"

bool Session::OnRead(DWORD bytesTransferred) {
	std::cout << "Received message from client: "
			  << std::string(readOv.buffer_, bytesTransferred) << std::endl;

	memmove(writeOv.buffer_ + writeOv.wsaBuf_.len, readOv.buffer_,
			bytesTransferred);

	writeOv.taskType_ = Task::SEND;
	writeOv.wsaBuf_.buf = writeOv.buffer_;
	writeOv.wsaBuf_.len += bytesTransferred;
	DWORD flags = 0;
	ZeroMemory(&writeOv.overlapped_, sizeof(OVERLAPPED));
	int sendResult = WSASend(socket_, &writeOv.wsaBuf_, 1, NULL, flags,
							 &writeOv.overlapped_, NULL);

	// WSASend가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
	// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
	if (sendResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSASend failed");
		return false;
	}

	return true;
}

bool Session::OnWrite(DWORD bytesTransferred) {
	std::cout << "Sent message to client: "
			  << std::string(writeOv.buffer_, bytesTransferred) << std::endl;

	writeOv.wsaBuf_.len -= bytesTransferred;
	if (writeOv.wsaBuf_.len > 0) {
		memmove(writeOv.buffer_, writeOv.buffer_ + bytesTransferred,
				writeOv.wsaBuf_.len);

		DWORD flags = 0;
		ZeroMemory(&writeOv.overlapped_, sizeof(OVERLAPPED));
		int sendResult = WSASend(socket_, &writeOv.wsaBuf_, 1, NULL, flags,
								 &writeOv.overlapped_, NULL);

		// WSASend가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
		// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
		if (sendResult == SOCKET_ERROR &&
			WSAGetLastError() != ERROR_IO_PENDING) {
			NetUtils::PrintError("WSASend failed");
			return false;
		}
	}

	DWORD flags = 0;
	int recvResult = WSARecv(socket_, &readOv.wsaBuf_, 1, NULL, &flags,
							 &readOv.overlapped_, NULL);
	// WSARecv가 즉시 완료되지 않으면 ERROR_IO_PENDING이 반환됨
	// 이 경우는 정상적인 상황이므로 오류로 처리하지 않음
	if (recvResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
		NetUtils::PrintError("WSARecv failed");
		return false;
	}

	return true;
}

bool Session::HandleIO(OverlappedEx<kReadBufferSize>* ovEx,
					   DWORD bytesTransferred) {
	switch (ovEx->taskType_) {
		case Task::RECV:
			return OnRead(bytesTransferred);
		case Task::SEND:
			return OnWrite(bytesTransferred);
		default:
			return false;
	}
}

void Session::Close() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}
