#pragma once

#include <memory>
#include <mutex>
#include <numeric>
#include <vector>

template <typename T>
class PoolElement : public std::enable_shared_from_this<T> {
   public:
   private:
};

template <typename T, size_t PoolSize = 1024>
	requires std::derived_from<T, PoolElement<T>>
class ObjectPool {
   public:
	ObjectPool() : freeList_(PoolSize) {
		std::iota(freeList_.begin(), freeList_.end(), 0);
	}

	std::shared_ptr<T> Acquire() {
		size_t index;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (freeList_.empty()) {
				return nullptr;
			}
			index = freeList_.back();
			freeList_.pop_back();
		}
		T* obj = new (pool_ + index * sizeof(T)) T();
		return std::shared_ptr<T>(obj, [this, index](T* ptr) {
			ptr->~T();
			std::lock_guard<std::mutex> lock(mutex_);
			freeList_.push_back(index);
		});
	}

   private:
	alignas(T) std::byte pool_[sizeof(T) * PoolSize];
	std::vector<size_t> freeList_;
	std::mutex mutex_;
};
