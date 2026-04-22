#pragma once

#include <WinSock2.h>

class MagicBuffer {
   public:
	MagicBuffer() = default;
	MagicBuffer(size_t size);
	~MagicBuffer();

	char* GetBuffer() { return buffer_; }
	size_t GetSize() const { return size_; }

   private:
	HANDLE hMap_ = NULL;
	size_t size_;
	char* buffer_;
};
