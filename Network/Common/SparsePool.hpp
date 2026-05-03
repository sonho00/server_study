#pragma once

#include <atomic>

#include "Logger.hpp"
#include "SharedPoolPtr.hpp"
#include "SparseSet.hpp"

template <typename T>
class ISparsePool {
   public:
	struct Slot {
		T obj_;
		uint64_t handle_{};
		std::atomic<size_t> refCount_;
	};

	virtual ~ISparsePool() = default;

	virtual bool AddRef(uint64_t handle) = 0;
	virtual bool ReleaseRef(uint64_t handle) = 0;

	[[nodiscard]] virtual bool IsValid(uint64_t handle) const = 0;

	[[nodiscard]] virtual T* Get(uint64_t handle) = 0;
	virtual bool Release(uint64_t handle) = 0;
};

template <typename T>
concept has_reset = requires(T obj) { obj.Reset(); };

template <typename T, size_t N, bool isLazy = false>
class SparsePool : public ISparsePool<T> {
   public:
	using Slot = typename ISparsePool<T>::Slot;

	SparsePool() {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				new (&pool_[i * sizeof(Slot)]) Slot();
			}
		}
	}

	~SparsePool() {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				auto& slot = reinterpret_cast<Slot&>(pool_[i * sizeof(Slot)]);
				slot.obj_.~T();
			}
		}
	}

	template <typename... Args>
	SharedPoolPtr<T> Acquire(Args&&... args) {
		uint64_t handle = sparseSet_.Pop();
		if (handle == SparseSet<N>::kInvalidHandle) {
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
	};

	[[nodiscard]] bool AddRef(uint64_t handle) override {
		if (!IsValid(handle)) {
			LOG_ERROR("Invalid handle: {}", handle);
			return false;
		}
		auto idx = static_cast<uint32_t>(handle);
		auto& slot = reinterpret_cast<Slot&>(pool_[idx * sizeof(Slot)]);
		slot.refCount_.fetch_add(1, std::memory_order_acq_rel);
		return true;
	}

	[[nodiscard]] bool ReleaseRef(uint64_t handle) override {
		if (!IsValid(handle)) {
			LOG_ERROR("Invalid handle: {}", handle);
			return false;
		}
		auto idx = static_cast<uint32_t>(handle);
		auto& slot = reinterpret_cast<Slot&>(pool_[idx * sizeof(Slot)]);
		if (slot.refCount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			if constexpr (isLazy) slot.obj_.~T();
			sparseSet_.Push(idx);
		}
		return true;
	}

	[[nodiscard]] bool IsValid(uint64_t handle) const override {
		return sparseSet_.IsValid(handle);
	}

	[[nodiscard]] T* Get(uint64_t handle) override {
		if (!IsValid(handle)) {
			LOG_ERROR("Invalid handle: {}", handle);
			return nullptr;
		}

		auto idx = static_cast<uint32_t>(handle);
		auto& slot = reinterpret_cast<Slot&>(pool_[idx * sizeof(Slot)]);
		return &slot.obj_;
	}

	bool Release(uint64_t handle) override { return sparseSet_.Push(handle); }

   private:
	alignas(Slot) std::array<std::byte, sizeof(Slot) * N> pool_{};
	SparseSet<N> sparseSet_;
};
