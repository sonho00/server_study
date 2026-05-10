#pragma once

#include <atomic>

template <typename T>
class ISparsePool {
   public:
	struct Slot {
		template <typename... Args>
		Slot(Args&&... args) : obj_(std::forward<Args>(args)...) {}
		T obj_;
		uint64_t handle_{};
		std::atomic<size_t> refCount_{0};
	};

	virtual bool AddRef(uint64_t handle) = 0;
	virtual bool ReleaseRef(uint64_t handle) = 0;

	[[nodiscard]] virtual bool IsValid(uint64_t handle) const = 0;

	[[nodiscard]] virtual T* Get(uint64_t handle) = 0;
	virtual bool Release(uint64_t handle) = 0;
};
