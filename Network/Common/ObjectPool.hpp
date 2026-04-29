#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <numeric>

template <typename T>
class PoolElement : public std ::enable_shared_from_this<T> {
	template <typename U, size_t N>
		requires std::derived_from<U, PoolElement<U>>
	friend class ObjectPool;

	uint64_t GetHandle() const {
		return (static_cast<uint64_t>(generation_) << 32) | who_;
	}

   private:
	size_t where_ = 0;
	size_t who_ = 0;
	size_t generation_ = 0;
};

template <typename T, size_t PoolSize = 1024>
	requires std::derived_from<T, PoolElement<T>>
class ObjectPool {
   public:
	ObjectPool() : activeCount_(0) {
		std::iota(std::begin(who_), std::end(who_), 0);
	}

	std::shared_ptr<T> Acquire();

   private:
	void Release(T* obj);

	alignas(T) std::byte pool_[sizeof(T) * PoolSize];
	size_t who_[PoolSize];
	size_t generation_[PoolSize];
	size_t activeCount_ = 0;
	std::mutex mutex_;
};

template <typename T, size_t PoolSize>
	requires std::derived_from<T, PoolElement<T>>
std::shared_ptr<T> ObjectPool<T, PoolSize>::Acquire() {
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
	obj->who_ = newPerson;
	obj->generation_ = generation_[newPerson];

	return std::shared_ptr<T>(obj, [this](T* obj) { Release(obj); });
}

template <typename T, size_t PoolSize>
	requires std::derived_from<T, PoolElement<T>>
void ObjectPool<T, PoolSize>::Release(T* obj) {
	std::lock_guard<std::mutex> lock(mutex_);
	--activeCount_;
	T* lastPerson =
		reinterpret_cast<T*>(pool_ + who_[activeCount_] * sizeof(T));
	lastPerson->where_ = obj->where_;
	std::swap(who_[obj->where_], who_[activeCount_]);
	++generation_[obj->who_];
	obj->~T();
}
