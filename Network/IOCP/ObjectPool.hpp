#pragma once

#include <memory>
#include <mutex>
#include <stack>

template <typename T>
class PoolElement : public std::enable_shared_from_this<T> {
   public:
	virtual ~PoolElement() = default;

	void Capture() { selfPtr_ = this->shared_from_this(); }
	void Release() { selfPtr_ = nullptr; }

	size_t poolIndex;

   private:
	std::shared_ptr<T> selfPtr_ = nullptr;
};

template <typename T, size_t PoolSize>
class ObjectPool {
   public:
	ObjectPool() {
		for (size_t i = 0; i < PoolSize; ++i) {
			freeIndices_.push(i);
		}
	}

	std::shared_ptr<T> Pop() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (freeIndices_.empty()) {
			return nullptr;
		}
		T* obj = pool_ + freeIndices_.top();
		freeIndices_.pop();
		std::shared_ptr<T> sharedObj(new (obj) T(),
									 [this](T* obj) { this->Push(obj); });
		sharedObj->Capture();
		return sharedObj;
	}

   private:
	void Push(T* obj) {
		std::lock_guard<std::mutex> lock(mutex_);
		size_t index = obj - pool_;
		obj->~T();
		if (index < PoolSize) {
			freeIndices_.push(index);
		}
	}

	T pool_[PoolSize];
	std::stack<size_t> freeIndices_;
	std::mutex mutex_;
};
