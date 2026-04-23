#pragma once

#include <WinSock2.h>
#include <mswsock.h>

namespace ServerUtils {
struct NetFuncs {
	NetFuncs();
	LPFN_ACCEPTEX AcceptEx;
};
SOCKET CreateListenSocket(const USHORT port);
}  // namespace ServerUtils
