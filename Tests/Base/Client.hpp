#pragma once

#include <WinSock2.h>

#include "Network/Common/Protocol.hpp"
#include "Network/Common/WSAManager.hpp"

class Client {
   public:
	~Client();

	void CreateSocket();

	[[nodiscard]] bool SendPacket(const PACKET_HEADER& header) const;
	[[nodiscard]] PACKET_HEADER* ReceivePacket(char* buffer);

	[[nodiscard]] bool SendByte(char* buffer, int len) const;
	[[nodiscard]] bool ReceiveByte(char* buffer, int len) const;

	static bool HandlePacket(const PACKET_HEADER& header);

	virtual void ThreadFunc() = 0;
	virtual bool test() = 0;

	void DefaultSockOpt();
	virtual void AdditionalSockOpt() {}
	virtual int Connect();

	bool success_ = true;

	static constexpr const char* kIpAddr_ = "127.0.0.1";
	static constexpr uint16_t kPort_ = 12345;

   private:
	WSAManager wsaManager_;

   protected:
	SOCKET socket_;
};
