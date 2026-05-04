#pragma once

#include <WinSock2.h>
#include <minwinbase.h>

class MagicBuffer {
   public:
	MagicBuffer(size_t size);
	~MagicBuffer();

	[[nodiscard]] char* GetBuffer() const { return buffer_; }
	[[nodiscard]] size_t GetSize() const { return size_; }

	void Clear() { ZeroMemory(buffer_, size_); }

   private:
	HANDLE hMap_;
	size_t size_;
	char* buffer_;
};
