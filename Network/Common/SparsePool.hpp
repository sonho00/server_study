#pragma once

#include <atomic>

#include "Network/Common/Logger.hpp"
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
	std::shared_ptr<T> Acquire(Args&&... args) {
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
		return std::shared_ptr<T>(&slot.obj_, [this, idx](T* ptr) {
			if constexpr (isLazy) ptr->~T();
			sparseSet_.Push(idx);
		});
	};

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
