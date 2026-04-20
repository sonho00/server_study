#pragma once

#include <WinSock2.h>
#include <Winsock2.h>
#include <mswsock.h>

#include <iostream>

namespace NetUtils {
bool Init();
SOCKET CreateListenSocket(USHORT port);
LPFN_ACCEPTEX GetAcceptEx(SOCKET listen_socket);
inline void PrintError(const char* msg) {
	int errorCode = WSAGetLastError();
	std::cerr << msg << " Error Code: " << errorCode << std::endl;
}
}  // namespace NetUtils
