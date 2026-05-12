#pragma once

#include <cstdint>

#include "ISparsePool.hpp"

template <typename T>
class SharedPoolPtr {
   public:
	SharedPoolPtr(ISparsePool<T>* pool, uint64_t handle);
	SharedPoolPtr(std::nullptr_t = nullptr);

	SharedPoolPtr(const SharedPoolPtr& other);
	SharedPoolPtr& operator=(const SharedPoolPtr& other);

	SharedPoolPtr(SharedPoolPtr&& other) noexcept;
	SharedPoolPtr& operator=(SharedPoolPtr&& other) noexcept;

	~SharedPoolPtr();

	[[nodiscard]] bool IsValid() const;

	[[nodiscard]] T* Get() const;
	[[nodiscard]] T& operator*() const;
	T* operator->();

	bool Reset();

	[[nodiscard]] uint64_t GetHandle() const { return handle_; }

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
SharedPoolPtr<T>::SharedPoolPtr(std::nullptr_t)
	: pool_(nullptr), handle_(ISparsePool<T>::kInvalidHandle) {}

template <typename T>
SharedPoolPtr<T>::SharedPoolPtr(const SharedPoolPtr& other)
	: pool_(other.pool_), handle_(other.handle_) {
	if (pool_) {
		pool_->AddRef(handle_);
	}
}

template <typename T>
SharedPoolPtr<T>& SharedPoolPtr<T>::operator=(const SharedPoolPtr& other) {
	if (this == &other) {
		return *this;
	}
	ISparsePool<T>* currentPool = pool_;
	uint64_t currentHandle = handle_;
	pool_ = other.pool_;
	handle_ = other.handle_;
	if (pool_) {
		pool_->AddRef(handle_);
	}
	if (currentPool) {
		currentPool->ReleaseRef(currentHandle);
	}
	return *this;
}

template <typename T>
SharedPoolPtr<T>::SharedPoolPtr(SharedPoolPtr&& other) noexcept
	: pool_(other.pool_), handle_(other.handle_) {
	other.pool_ = nullptr;
	other.handle_ = ISparsePool<T>::kInvalidHandle;
}

template <typename T>
SharedPoolPtr<T>& SharedPoolPtr<T>::operator=(SharedPoolPtr&& other) noexcept {
	if (this != &other) {
		if (pool_) pool_->ReleaseRef(handle_);

		pool_ = other.pool_;
		handle_ = other.handle_;

		other.pool_ = nullptr;
		other.handle_ = ISparsePool<T>::kInvalidHandle;
	}
	return *this;
}

template <typename T>
SharedPoolPtr<T>::~SharedPoolPtr() {
	if (pool_ && pool_->IsValid(handle_)) {
		pool_->ReleaseRef(handle_);
	}
}

template <typename T>
bool SharedPoolPtr<T>::IsValid() const {
	return pool_ && pool_->IsValid(handle_);
}

template <typename T>
T* SharedPoolPtr<T>::Get() const {
	return IsValid() ? pool_->GetObj(handle_) : nullptr;
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
bool SharedPoolPtr<T>::Reset() {
	if (pool_ && pool_->IsValid(handle_)) {
		if (pool_->ReleaseRef(handle_)) {
			return true;
		}
		pool_ = nullptr;
		handle_ = ISparsePool<T>::kInvalidHandle;
	}
	return false;
}
