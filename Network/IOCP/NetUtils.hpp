#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include <iostream>

namespace NetUtils {
struct NetFuncs {
	NetFuncs();
	LPFN_ACCEPTEX AcceptEx;
};
SOCKET CreateListenSocket(USHORT port);
inline void PrintError(const char* msg, int errorCode = WSAGetLastError()) {
	std::cerr << msg << " Error Code: " << errorCode << std::endl;
}
}  // namespace NetUtils
