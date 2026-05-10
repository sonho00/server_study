#pragma once

#include <array>
#include <cstddef>

template <typename T>
concept has_reset = requires(T obj) { obj.Reset(); };

template <typename T, size_t N, bool isLazy = false>
class ObjectPool {
   public:
	ObjectPool() {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				new (&pool_[i * sizeof(T)]) T();
			}
		}
	}

	~ObjectPool() {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				auto* obj = reinterpret_cast<T*>(&pool_[i * sizeof(T)]);
				obj->~T();
			}
		}
	}

	template <typename... Args>
	T* Acquire(size_t idx, Args&&... args) {
		T* obj = reinterpret_cast<T*>(&pool_[idx * sizeof(T)]);

		if constexpr (isLazy) {
			new (obj) T(std::forward<Args>(args)...);
		} else if constexpr (has_reset<T>) {
			obj->Reset(std::forward<Args>(args)...);
		}

		return obj;
	};

	bool Release(size_t idx) {
		T* obj = reinterpret_cast<T*>(&pool_[idx * sizeof(T)]);
		if constexpr (isLazy) {
			obj->~T();
		}
		return true;
	}

	T* Get(size_t idx) { return reinterpret_cast<T*>(&pool_[idx * sizeof(T)]); }

   private:
	alignas(T) std::array<std::byte, sizeof(T) * N> pool_;
};
