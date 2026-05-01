#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <mutex>
#include <numeric>
#include <optional>
#include <vector>

#include "Logger.hpp"

struct Slot {
	size_t where_ = 0;
	size_t who_ = 0;
	size_t generation_ = 0;
};

template <size_t N>
class SparseSet {
   public:
	SparseSet() { std::iota(std::begin(who_), std::end(who_), 0); }

	[[nodiscard]] std::optional<uint64_t> Pop();
	[[nodiscard]] bool Pop(uint64_t handle);

	bool Push(uint64_t handle);

	[[nodiscard]] std::vector<uint64_t> GetActiveIndices();

   private:
	std::array<Slot, N> slots_;
	std::array<uint32_t, N> who_;
	std::array<uint32_t, N> generation_{};
	size_t activeCount_ = 0;
	std::mutex mutex_;
};

template <size_t N>
[[nodiscard]] std::optional<uint64_t> SparseSet<N>::Pop() {
	size_t newPerson;
	Slot* slot = nullptr;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (activeCount_ >= N) {
			LOG_DEBUG("No available slots: activeCount_: {}, N: {}",
					  activeCount_, N);
			return std::nullopt;
		}

		newPerson = who_[activeCount_];
		slot = &slots_[newPerson];
		slot->where_ = activeCount_;
		++activeCount_;
	}

	slot->who_ = newPerson;
	slot->generation_ = generation_[newPerson];

	return (static_cast<uint64_t>(slot->generation_)
			<< (sizeof(uint32_t) * CHAR_BIT)) |
		   slot->who_;
}

template <size_t N>
[[nodiscard]] bool SparseSet<N>::Pop(uint64_t handle) {
	size_t who = handle & std::numeric_limits<uint32_t>::max();
	size_t generation = handle >> (sizeof(uint32_t) * CHAR_BIT);

	if (who >= N || slots_[who].generation_ != generation) {
		LOG_ERROR(
			"Invalid handle: who: {}, generation: {}, "
			"slots_[who].generation_: {}",
			who, generation, slots_[who].generation_);
		return false;  // Invalid handle
	}

	std::lock_guard<std::mutex> lock(mutex_);
	if (activeCount_ >= N) {
		return false;  // Invalid handle
	}

	slots_[who].where_ = activeCount_;
	who_[activeCount_] = who;
	++activeCount_;
	return true;
}

template <size_t N>
bool SparseSet<N>::Push(uint64_t handle) {
	size_t who = handle & std::numeric_limits<uint32_t>::max();
	size_t generation = handle >> (sizeof(uint32_t) * CHAR_BIT);

	std::lock_guard<std::mutex> lock(mutex_);
	if (who >= N || generation_[who] != generation) {
		LOG_ERROR(
			"Invalid handle: who: {}, generation: {}, "
			"generation_[who]: {}",
			who, generation, generation_[who]);
		return false;  // Invalid handle
	}

	--activeCount_;
	Slot& lastPerson = slots_[who_[activeCount_]];
	lastPerson.where_ = slots_[who].where_;
	std::swap(who_[lastPerson.where_], who_[activeCount_]);
	++generation_[who];

	return true;
}

template <size_t N>
[[nodiscard]] std::vector<uint64_t> SparseSet<N>::GetActiveIndices() {
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<uint64_t> activeIndicies;
	activeIndicies.insert(activeIndicies.end(), who_.begin(),
						  who_.begin() + activeCount_);
	return activeIndicies;
}
