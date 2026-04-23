#pragma once

#include <WinSock2.h>

class MagicBuffer {
   public:
	MagicBuffer() = default;
	MagicBuffer(const size_t size);
	~MagicBuffer();

	char* GetBuffer() const { return buffer_; }
	size_t GetSize() const { return size_; }

   private:
	HANDLE hMap_;
	size_t size_;
	char* buffer_;
};
