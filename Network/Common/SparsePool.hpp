#pragma once

#include <atomic>

#include "ISparsePool.hpp"
#include "Logger.hpp"
#include "SharedPoolPtr.hpp"
#include "SparseSet.hpp"

template <typename T>
concept has_reset = requires(T obj) { obj.Reset(); };

template <typename T, size_t N, bool isLazy = false>
class SparsePool : public ISparsePool<T> {
   public:
	using Slot = typename ISparsePool<T>::Slot;

	SparsePool();
	~SparsePool();

	template <typename... Args>
	SharedPoolPtr<T> Acquire(Args&&... args);
	bool Release(uint64_t handle) override;

	bool AddRef(uint64_t handle) override;
	bool ReleaseRef(uint64_t handle) override;

	[[nodiscard]] bool IsValid(uint64_t handle) const override;
	[[nodiscard]] T* Get(uint64_t handle) override;

   private:
	alignas(Slot) std::array<std::byte, sizeof(Slot) * N> pool_{};
	SparseSet<N> sparseSet_;
};

template <typename T, size_t N, bool isLazy>
SparsePool<T, N, isLazy>::SparsePool() {
	if constexpr (!isLazy) {
		for (size_t i = 0; i < N; ++i) {
			new (&pool_[i * sizeof(Slot)]) Slot();
		}
	}
}

template <typename T, size_t N, bool isLazy>
SparsePool<T, N, isLazy>::~SparsePool() {
	if constexpr (!isLazy) {
		for (size_t i = 0; i < N; ++i) {
			auto& slot = reinterpret_cast<Slot&>(pool_[i * sizeof(Slot)]);
			slot.obj_.~T();
		}
	}
}

template <typename T, size_t N, bool isLazy>
template <typename... Args>
SharedPoolPtr<T> SparsePool<T, N, isLazy>::Acquire(Args&&... args) {
	uint64_t handle = sparseSet_.Pop();
	if (!IsValid(handle)) {
		return nullptr;
	}
	auto idx = static_cast<uint32_t>(handle);

	auto& slot = reinterpret_cast<Slot&>(pool_[idx * sizeof(Slot)]);

	if constexpr (isLazy) {
		new (&slot.obj_) T(std::forward<Args>(args)...);
	} else if constexpr (has_reset<T>) {
		slot.obj_.Reset(std::forward<Args>(args)...);
	}
	slot.obj_.SetHandle(handle);
	return SharedPoolPtr<T>(this, handle);
}

template <typename T, size_t N, bool isLazy>
bool SparsePool<T, N, isLazy>::AddRef(uint64_t handle) {
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return false;
	}
	auto idx = static_cast<uint32_t>(handle);
	auto& slot = reinterpret_cast<Slot&>(pool_[idx * sizeof(Slot)]);
	slot.refCount_.fetch_add(1, std::memory_order_acq_rel);
	return true;
}

template <typename T, size_t N, bool isLazy>
bool SparsePool<T, N, isLazy>::ReleaseRef(uint64_t handle) {
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return false;
	}
	auto idx = static_cast<uint32_t>(handle);
	auto& slot = reinterpret_cast<Slot&>(pool_[idx * sizeof(Slot)]);
	if (slot.refCount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
		if constexpr (isLazy) slot.obj_.~T();
		sparseSet_.Push(handle);
		return true;
	}
	return false;
}
template <typename T, size_t N, bool isLazy>
bool SparsePool<T, N, isLazy>::IsValid(uint64_t handle) const {
	return sparseSet_.IsValid(handle);
}

template <typename T, size_t N, bool isLazy>
T* SparsePool<T, N, isLazy>::Get(uint64_t handle) {
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return nullptr;
	}

	auto idx = static_cast<uint32_t>(handle);
	auto& slot = reinterpret_cast<Slot&>(pool_[idx * sizeof(Slot)]);
	return &slot.obj_;
}

template <typename T, size_t N, bool isLazy>
bool SparsePool<T, N, isLazy>::Release(uint64_t handle) {
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return false;
	}
	ReleaseRef(handle);
	return true;
}
