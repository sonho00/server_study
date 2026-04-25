#pragma once

#include <WinSock2.h>

#include <iostream>
#include <string>

namespace NetUtils {
inline void PrintError(const char* msg, int errorCode = WSAGetLastError()) {
	std::string s =
		std::string(msg) + " Error Code: " + std::to_string(errorCode) + "\n";
	std::cerr << s;
}
}  // namespace NetUtils
