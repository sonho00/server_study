#pragma once

#include <array>
#include <memory>

template <typename T, size_t N>
	requires std::derived_from<T, std::enable_shared_from_this<T>>
class ObjectPool {
   public:
	template <typename... Args>
	std::shared_ptr<T> Acquire(size_t idx, Args&&... args) {
		T* obj = new (&pool_[idx * sizeof(T)]) T(std::forward<Args>(args)...);
		return std::shared_ptr<T>(obj, [this, idx](T* ptr) { ptr->~T(); });
	};

   private:
	alignas(T) std::array<std::byte, sizeof(T) * N> pool_;
};
