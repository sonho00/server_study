#pragma once
#include <WinSock2.h>

#include <iostream>

namespace NetUtils {
inline void PrintError(const char* msg, int errorCode = WSAGetLastError()) {
	std::cerr << msg << " Error Code: " << errorCode << std::endl;
}
}  // namespace NetUtils
