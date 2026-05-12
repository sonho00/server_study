#pragma once

#include <atomic>

#include "ISparsePool.hpp"
#include "Network/Common/Logger.hpp"
#include "ObjectPool.hpp"
#include "SharedPoolPtr.hpp"
#include "SparseSet.hpp"

template <typename T, size_t N, size_t StateCount = 2, bool isLazy = false>
class SparsePool : public ISparsePool<T>, public SparseSet<N, StateCount> {
	using Slot = ISparsePool<T>::Slot;
	using deleteFunc = std::function<void(T*)>;

   public:
	SparsePool(deleteFunc deleter = nullptr)
		: pool_([deleter](Slot* slot) {
			  if (deleter) {
				  deleter(&slot->obj_);
			  }
		  }) {}

	template <typename... Args>
	SharedPoolPtr<T> Acquire(size_t state = 1, Args&&... args);

	bool AddRef(uint64_t handle) override;
	bool ReleaseRef(uint64_t handle) override;

	[[nodiscard]] bool IsValid(uint64_t handle) const override;

	[[nodiscard]] T* GetObj(uint64_t handle) override;

   private:
	ObjectPool<Slot, N, isLazy> pool_;
};

template <typename T, size_t N, size_t StateCount, bool isLazy>
template <typename... Args>
SharedPoolPtr<T> SparsePool<T, N, StateCount, isLazy>::Acquire(size_t state,
															   Args&&... args) {
	uint64_t handle = SparseSet<N, StateCount>::Pop(state);
	if (!IsValid(handle)) {
		return nullptr;
	}

	auto idx = static_cast<uint32_t>(handle);
	Slot* slot = pool_.Acquire(idx, std::forward<Args>(args)...);

	slot->handle_ = handle;
	SparseSet<N, StateCount>::MoveToState(handle, state);

	return SharedPoolPtr<T>(this, handle);
}

template <typename T, size_t N, size_t StateCount, bool isLazy>
bool SparsePool<T, N, StateCount, isLazy>::AddRef(uint64_t handle) {
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return false;
	}

	auto idx = static_cast<uint32_t>(handle);
	Slot* slot = pool_.Get(idx);
	slot->refCount_.fetch_add(1, std::memory_order_acq_rel);

	return true;
}

template <typename T, size_t N, size_t StateCount, bool isLazy>
bool SparsePool<T, N, StateCount, isLazy>::ReleaseRef(uint64_t handle) {
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return false;
	}

	auto idx = static_cast<uint32_t>(handle);
	Slot* slot = pool_.Get(idx);
	if (slot->refCount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
		SparseSet<N, StateCount>::Push(handle);
		pool_.Release(idx);
		return true;
	}

	return false;
}

template <typename T, size_t N, size_t StateCount, bool isLazy>
bool SparsePool<T, N, StateCount, isLazy>::IsValid(uint64_t handle) const {
	return SparseSet<N, StateCount>::IsValid(handle);
}

template <typename T, size_t N, size_t StateCount, bool isLazy>
T* SparsePool<T, N, StateCount, isLazy>::GetObj(uint64_t handle) {
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return nullptr;
	}

	auto idx = static_cast<uint32_t>(handle);
	Slot* slot = pool_.Get(idx);

	return &slot->obj_;
}
