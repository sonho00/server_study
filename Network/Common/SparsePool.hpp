#pragma once

#include <atomic>

#include "SparseSet.hpp"

template <typename T, size_t N, bool isLazy = false>
class SparsePool {
	struct Slot {
		T obj_;
		uint64_t handle_{};
		std::atomic<size_t> refCount_;
	};

   public:
	SparsePool() {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				new (&pool_[i * sizeof(T)]) T();
			}
		}
	}

	template <typename... Args>
	std::shared_ptr<T> Acquire(Args&&... args) {
		uint64_t handle = sparseSet_.Pop();
		if (handle == SparseSet<N>::kInvalidHandle) {
			return nullptr;
		}
		auto idx = static_cast<uint32_t>(handle);

		auto& obj = reinterpret_cast<T&>(pool_[idx * sizeof(T)]);
		if constexpr (isLazy) obj = T(std::forward<Args>(args)...);
		obj.SetHandle(handle);
		return std::shared_ptr<T>(&obj, [this, idx](T* ptr) {
			if constexpr (isLazy) ptr->~T();
			sparseSet_.Push(idx);
		});
	};

	bool Release(uint64_t handle) { return sparseSet_.Push(handle); }

   private:
	alignas(Slot) std::array<std::byte, sizeof(Slot) * N> pool_;
	SparseSet<N> sparseSet_;
};
