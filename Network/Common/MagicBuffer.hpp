#pragma once

#include <WinSock2.h>

class MagicBuffer {
   public:
	MagicBuffer(size_t size);
	~MagicBuffer();

	[[nodiscard]] char* GetBuffer() const { return buffer_; }
	[[nodiscard]] size_t GetSize() const { return size_; }

   private:
	HANDLE hMap_;
	size_t size_;
	char* buffer_;
};
