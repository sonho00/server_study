#pragma once

#include <cstdint>

template <typename T>
class ISparsePool;

template <typename T>
class SharedPoolPtr {
   public:
	SharedPoolPtr(ISparsePool<T>* pool, uint64_t handle)
		: pool_(pool), handle_(handle) {
		pool_->AddRef(handle_);
	}

	SharedPoolPtr(const SharedPoolPtr& other)
		: pool_(other.pool_), handle_(other.handle_) {
		pool_->AddRef(handle_);
	}

	SharedPoolPtr(SharedPoolPtr&& other) noexcept
		: pool_(other.pool_), handle_(other.handle_) {
		other.pool_ = nullptr;
		other.handle_ = 0;
	}

	SharedPoolPtr(std::nullptr_t = nullptr) : pool_(nullptr), handle_(0) {}

	~SharedPoolPtr() {
		if (pool_) {
			pool_->ReleaseRef(handle_);
		}
	}

	SharedPoolPtr& operator=(const SharedPoolPtr& other) {
		if (this != &other) {
			if (pool_) {
				pool_->ReleaseRef(handle_);
			}
			pool_ = other.pool_;
			handle_ = other.handle_;
			if (pool_) {
				pool_->AddRef(handle_);
			}
		}
		return *this;
	}

	[[nodiscard]] explicit operator bool() const {
		return pool_ && pool_->IsValid(handle_);
	}

	[[nodiscard]] T* Get() const { return pool_->Get(handle_); }

	T* operator->() { return pool_->Get(handle_); }

	[[nodiscard]] T& operator*() const { return *pool_->Get(handle_); }

	void Reset() {
		pool_ = nullptr;
		handle_ = 0;
	}

   private:
	ISparsePool<T>* pool_;
	uint64_t handle_;
};
