#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <iostream>

namespace NetUtils {
bool Init();
SOCKET CreateListenSocket(USHORT port);
bool GetAcceptEx();
inline void PrintError(const char* msg, int errorCode = WSAGetLastError()) {
	std::cerr << msg << " Error Code: " << errorCode << std::endl;
}
extern LPFN_ACCEPTEX AcceptEx;
}  // namespace NetUtils
