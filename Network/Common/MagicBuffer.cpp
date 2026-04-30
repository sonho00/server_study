#include "MagicBuffer.hpp"

#include "NetUtils.hpp"

MagicBuffer::MagicBuffer(size_t size) : size_(size) {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	if (size_ % sysInfo.dwAllocationGranularity != 0) {
		LOG_FATAL(
			"Buffer size must be a multiple of system allocation granularity "
			"({} bytes)",
			sysInfo.dwAllocationGranularity);
	}

	// 1. 페이지 파일 매핑 객체 생성
	hMap_ = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
							  static_cast<DWORD>(size_), nullptr);
	if (hMap_ == nullptr) {
		LOG_FATAL("Failed to create file mapping: {}", GetLastError());
	}

	// 2. 연속된 가상 주소 공간을 2배 크기로 예약
	void* ptr = static_cast<char*>(
		VirtualAlloc(nullptr, size_ * 2, MEM_RESERVE, PAGE_NOACCESS));
	if (ptr == nullptr) {
		CloseHandle(hMap_);
		LOG_FATAL("Failed to reserve virtual address space: {}",
				  GetLastError());
	}

	VirtualFree(ptr, 0, MEM_RELEASE);

	buffer_ = static_cast<char*>(ptr);

	// 3. 예약된 주소 공간의 앞부분(0 ~ size)에 물리 메모리 매핑
	if (MapViewOfFileEx(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, size_, buffer_) ==
		nullptr) {
		LOG_FATAL("Failed to map view of file to first half: {}",
				  GetLastError());
	}

	// 4. 예약된 주소 공간의 뒷부분(size ~ 2*size)에 동일한 물리 메모리 매핑
	if (MapViewOfFileEx(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, size_,
						buffer_ + size_) == nullptr) {
		LOG_FATAL("Failed to map view of file to second half: {}",
				  GetLastError());
	}
}

MagicBuffer::~MagicBuffer() {
	if (buffer_ != nullptr) {
		UnmapViewOfFile(buffer_);
		UnmapViewOfFile(buffer_ + size_);
		VirtualFree(buffer_, 0, MEM_RELEASE);
	}
	if (hMap_ != nullptr) CloseHandle(hMap_);
}
