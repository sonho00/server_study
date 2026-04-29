#pragma once

#include <memory>
#include <mutex>
#include <numeric>

template <typename T>
class PoolElement : public std ::enable_shared_from_this<T> {
	template <typename U, size_t N>
		requires std::derived_from<U, PoolElement<U>>
	friend class ObjectPool;

   private:
	size_t where_ = 0;
};

template <typename T, size_t PoolSize = 1024>
	requires std::derived_from<T, PoolElement<T>>
class ObjectPool {
   public:
	ObjectPool() : activeCount_(0) {
		std::iota(std::begin(who_), std::end(who_), 0);
	}

	std::shared_ptr<T> Acquire() {
		size_t newPerson;
		T* obj = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (activeCount_ >= PoolSize) {
				return nullptr;
			}
			newPerson = who_[activeCount_];
			obj = new (pool_ + newPerson * sizeof(T)) T();
			obj->where_ = activeCount_;
			++activeCount_;
		}

		return std::shared_ptr<T>(obj, [this, newPerson](T* ptr) {
			std::lock_guard<std::mutex> lock(mutex_);
			--activeCount_;
			T* lastPerson =
				reinterpret_cast<T*>(pool_ + who_[activeCount_] * sizeof(T));
			lastPerson->where_ = ptr->where_;
			std::swap(who_[ptr->where_], who_[activeCount_]);
			ptr->~T();
		});
	}

   private:
	alignas(T) std::byte pool_[sizeof(T) * PoolSize];
	size_t who_[PoolSize];
	size_t activeCount_ = 0;
	std::mutex mutex_;
};
