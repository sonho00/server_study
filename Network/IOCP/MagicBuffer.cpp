#include "MagicBuffer.hpp"

#include <stdexcept>

#include "Network/IOCP/NetUtils.hpp"

MagicBuffer::MagicBuffer(size_t size) : size_(size) {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	if (size_ % sysInfo.dwAllocationGranularity != 0) {
		NetUtils::PrintError(
			"Buffer size must be a multiple of the system's allocation "
			"granularity");
		throw std::runtime_error("Invalid buffer size");
	}

	// 1. 페이지 파일 매핑 객체 생성
	hMap_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
							  static_cast<DWORD>(size_), NULL);
	if (!hMap_) throw std::runtime_error("Failed to create file mapping");

	// 2. 연속된 가상 주소 공간을 2배 크기로 예약
	void* ptr = static_cast<char*>(
		VirtualAlloc(NULL, size_ * 2, MEM_RESERVE, PAGE_NOACCESS));
	if (!ptr) throw std::runtime_error("Failed to reserve virtual memory");

	VirtualFree(ptr, 0, MEM_RELEASE);

	buffer_ = static_cast<char*>(ptr);

	// 3. 예약된 주소 공간의 앞부분(0 ~ size)에 물리 메모리 매핑
	if (!MapViewOfFileEx(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, size_, buffer_)) {
		throw std::runtime_error("Failed to map view of file to first half");
	}

	// 4. 예약된 주소 공간의 뒷부분(size ~ 2*size)에 동일한 물리 메모리 매핑
	if (!MapViewOfFileEx(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, size_,
						 buffer_ + size_)) {
		throw std::runtime_error("Failed to map view of file to second half");
	}
}

MagicBuffer::~MagicBuffer() {
	if (buffer_) {
		UnmapViewOfFile(buffer_);
		UnmapViewOfFile(buffer_ + size_);
		VirtualFree(buffer_, 0, MEM_RELEASE);
	}
	if (hMap_) CloseHandle(hMap_);
}
