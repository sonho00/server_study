#pragma once

#include <cstdint>

#include "ISparsePool.hpp"

template <typename T>
class SharedPoolPtr {
   public:
	SharedPoolPtr(ISparsePool<T>* pool, uint64_t handle);
	SharedPoolPtr(const SharedPoolPtr& other);
	SharedPoolPtr(SharedPoolPtr&& other) noexcept;
	SharedPoolPtr(std::nullptr_t = nullptr);
	SharedPoolPtr& operator=(const SharedPoolPtr& other);

	~SharedPoolPtr();

	[[nodiscard]] explicit operator bool() const;

	[[nodiscard]] T* Get() const;
	[[nodiscard]] T& operator*() const;
	T* operator->();

	void Reset();

   private:
	ISparsePool<T>* pool_;
	uint64_t handle_;
};

template <typename T>
SharedPoolPtr<T>::SharedPoolPtr(ISparsePool<T>* pool, uint64_t handle)
	: pool_(pool), handle_(handle) {
	if (pool_) {
		pool_->AddRef(handle_);
	}
}

template <typename T>
SharedPoolPtr<T>::SharedPoolPtr(const SharedPoolPtr& other)
	: pool_(other.pool_), handle_(other.handle_) {
	if (pool_) {
		pool_->AddRef(handle_);
	}
}

template <typename T>
SharedPoolPtr<T>::SharedPoolPtr(SharedPoolPtr&& other) noexcept
	: pool_(other.pool_), handle_(other.handle_) {
	other.pool_ = nullptr;
	other.handle_ = 0;
}

template <typename T>
SharedPoolPtr<T>::SharedPoolPtr(std::nullptr_t) : pool_(nullptr), handle_(0) {}

template <typename T>
SharedPoolPtr<T>& SharedPoolPtr<T>::operator=(const SharedPoolPtr& other) {
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

template <typename T>
SharedPoolPtr<T>::~SharedPoolPtr() {
	if (pool_) {
		pool_->ReleaseRef(handle_);
	}
}

template <typename T>
SharedPoolPtr<T>::operator bool() const {
	return pool_ && pool_->IsValid(handle_);
}

template <typename T>
T* SharedPoolPtr<T>::Get() const {
	return pool_ ? pool_->Get(handle_) : nullptr;
}

template <typename T>
T& SharedPoolPtr<T>::operator*() const {
	return *Get();
}

template <typename T>
T* SharedPoolPtr<T>::operator->() {
	return Get();
}

template <typename T>
void SharedPoolPtr<T>::Reset() {
	if (pool_) {
		pool_->ReleaseRef(handle_);
	}
	pool_ = nullptr;
	handle_ = 0;
}
