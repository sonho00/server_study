#pragma once

#include <atomic>

template <typename T>
class ISparsePool {
   public:
	struct Slot {
		template <typename... Args>
		Slot(Args&&... args) : obj_(std::forward<Args>(args)...) {}
		T obj_;
		uint64_t handle_ = kInvalidHandle;
		std::atomic<size_t> refCount_{0};
	};

	static constexpr uint64_t kInvalidHandle = static_cast<uint64_t>(-1);

	virtual bool AddRef(uint64_t handle) = 0;
	virtual bool ReleaseRef(uint64_t handle) = 0;

	[[nodiscard]] virtual bool IsValid(uint64_t handle) const = 0;

	[[nodiscard]] virtual T* GetObj(uint64_t handle) = 0;
};
