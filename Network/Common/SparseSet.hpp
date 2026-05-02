#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <numeric>
#include <vector>

#include "Logger.hpp"

template <size_t N>
class SparseSet {
   public:
	struct Slot {
		uint32_t where_ = 0;
		uint32_t generation_ = 0;
	};

	static constexpr uint64_t kInvalidHandle = static_cast<uint64_t>(-1);

	SparseSet() { std::iota(std::begin(dense_), std::end(dense_), 0); }

	[[nodiscard]] uint64_t Pop();
	bool Pop(uint64_t handle);
	bool Push(uint64_t handle);

	[[nodiscard]] std::vector<uint64_t> GetActiveIndices();

   private:
	static constexpr uint8_t kIndexShift = 32;

	std::array<Slot, N> sparse_;
	std::array<uint32_t, N> dense_;
	std::array<uint32_t, N> generation_{};
	size_t activeCount_ = 0;
	std::mutex mutex_;
};

template <size_t N>
uint64_t SparseSet<N>::Pop() {
	uint32_t newPerson;
	Slot* slot = nullptr;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (activeCount_ >= N) {
			LOG_DEBUG("No available slots: activeCount_: {}, N: {}",
					  activeCount_, N);
			return kInvalidHandle;
		}

		newPerson = dense_[activeCount_];
		slot = &sparse_[newPerson];
		slot->where_ = activeCount_;
		++activeCount_;
	}

	slot->generation_ = generation_[newPerson];

	return (static_cast<uint64_t>(generation_[newPerson]) << kIndexShift) |
		   newPerson;
}

template <size_t N>
bool SparseSet<N>::Pop(uint64_t handle) {
	auto who = static_cast<uint32_t>(handle);
	auto generation = static_cast<uint32_t>(handle >> kIndexShift);

	if (who >= N || sparse_[who].generation_ != generation) {
		LOG_ERROR(
			"Invalid handle: who: {}, generation: {}, "
			"slots_[who].generation_: {}",
			who, generation, sparse_[who].generation_);
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	sparse_[who].where_ = activeCount_;
	dense_[activeCount_] = who;
	++activeCount_;
	return true;
}

template <size_t N>
bool SparseSet<N>::Push(uint64_t handle) {
	auto who = static_cast<uint32_t>(handle);
	auto generation = static_cast<uint32_t>(handle >> kIndexShift);

	std::lock_guard<std::mutex> lock(mutex_);
	if (who >= N || generation_[who] != generation) {
		LOG_ERROR(
			"Invalid handle: who: {}, generation: {}, "
			"generation_[who]: {}",
			who, generation, generation_[who]);
		return false;
	}

	--activeCount_;
	Slot& lastPerson = sparse_[dense_[activeCount_]];
	lastPerson.where_ = sparse_[who].where_;
	std::swap(dense_[lastPerson.where_], dense_[activeCount_]);
	++generation_[who];

	return true;
}

template <size_t N>
[[nodiscard]] std::vector<uint64_t> SparseSet<N>::GetActiveIndices() {
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<uint64_t> activeIndicies;
	activeIndicies.insert(activeIndicies.end(), dense_.begin(),
						  dense_.begin() + activeCount_);
	return activeIndicies;
}
