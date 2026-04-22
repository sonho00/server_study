#pragma once

#include <memory>
#include <mutex>
#include <numeric>

template <typename T>
class PoolElement : public std::enable_shared_from_this<T> {
   public:
	virtual ~PoolElement() = default;

	void Capture() { selfPtr_ = this->shared_from_this(); }
	void Release() { selfPtr_.reset(); }

	void SetPoolIdx(size_t idx) { poolIdx_ = idx; }
	size_t GetPoolIdx() const { return poolIdx_; }

   private:
	size_t poolIdx_ = 0;
	std::shared_ptr<T> selfPtr_ = nullptr;
};

template <typename T, size_t PoolSize = 1024>
class ObjectPool {
   public:
	ObjectPool() { std::iota(std::begin(idx_), std::end(idx_), 0); }

	std::shared_ptr<T> Acquire() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (count_ == PoolSize) return nullptr;

		size_t objectPos = idx_[count_];
		T* object = &pool_[objectPos];

		object->SetPoolIdx(count_);

		auto sptr =
			std::shared_ptr<T>(object, [this](T* obj) { this->Release(obj); });
		sptr->Capture();

		++count_;
		return sptr;
	}

	void Release(T* object) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (count_ == 0) return;

		size_t currentPos = object->GetPoolIdx();
		size_t lastObjectPos = idx_[--count_];
		
		std::swap(idx_[currentPos], idx_[count_]);
		pool_[lastObjectPos].SetPoolIdx(currentPos);

		object->Release();
	}

   private:
	T pool_[PoolSize];
	size_t idx_[PoolSize];
	size_t count_ = 0;
	std::mutex mutex_;
};
