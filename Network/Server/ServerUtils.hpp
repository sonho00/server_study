#pragma once

#include <WinSock2.h>
#include <mswsock.h>

namespace ServerUtils {
struct NetFuncs {
	NetFuncs();
	LPFN_ACCEPTEX acceptEx_;
};
SOCKET CreateListenSocket(USHORT port);
}  // namespace ServerUtils
