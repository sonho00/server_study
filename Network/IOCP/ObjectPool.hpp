#pragma once

#include <mutex>
#include <stack>

template <typename T, size_t PoolSize>
class ObjectPool {
   public:
	ObjectPool() {
		for (size_t i = 0; i < PoolSize; ++i) {
			freeIndices_.push(i);
		}
	}

	void Push(T* obj) {
		std::lock_guard<std::mutex> lock(mutex_);
		size_t index = obj - pool_;
		if (index < PoolSize) {
			obj->~T();
			freeIndices_.push(index);
		}
	}

	T* Pop() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (freeIndices_.empty()) {
			return nullptr;
		}
		T* obj = &pool_[freeIndices_.top()];
		freeIndices_.pop();
		return new (obj) T();
	}

   private:
	T pool_[PoolSize];
	std::stack<size_t> freeIndices_;
	std::mutex mutex_;
};
