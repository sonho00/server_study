#pragma once

#include <array>
#include <memory>

template <typename T, size_t N, bool isLazy = false>
	requires std::derived_from<T, std::enable_shared_from_this<T>>
class ObjectPool {
   public:
	ObjectPool() {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				new (&pool_[i * sizeof(T)]) T();
			}
		}
	}

	template <typename... Args>
	std::shared_ptr<T> Acquire(size_t idx, Args&&... args) {
		T* obj = reinterpret_cast<T*>(&pool_[idx * sizeof(T)]);
		if constexpr (isLazy) {
			obj = new (obj) T(std::forward<Args>(args)...);
		}

		return std::shared_ptr<T>(obj, [this, idx](T* ptr) {
			if constexpr (isLazy) ptr->~T();
		});
	};

   private:
	alignas(T) std::array<std::byte, sizeof(T) * N> pool_;
};
