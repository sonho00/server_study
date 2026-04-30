#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <mutex>
#include <numeric>
#include <optional>
#include <vector>

struct Slot {
	size_t where_ = 0;
	size_t who_ = 0;
	size_t generation_ = 0;
};

template <size_t N>
class SparseSet {
   public:
	SparseSet() { std::iota(std::begin(who_), std::end(who_), 0); }

	[[nodiscard]] std::optional<uint64_t> Pop() {
		size_t newPerson;
		Slot* slot = nullptr;

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (activeCount_ >= N) {
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

	bool Push(uint64_t handle) {
		size_t who = handle & std::numeric_limits<uint32_t>::max();
		size_t generation = handle >> (sizeof(uint32_t) * CHAR_BIT);

		std::lock_guard<std::mutex> lock(mutex_);
		if (who >= N || generation_[who] != generation) {
			return false;  // Invalid handle
		}

		--activeCount_;
		Slot& lastPerson = slots_[who_[activeCount_]];
		lastPerson.where_ = slots_[who].where_;
		std::swap(who_[lastPerson.where_], who_[activeCount_]);
		++generation_[who];

		return true;
	}

	[[nodiscard]] std::vector<uint64_t> GetActiveIndices() {
		std::vector<uint64_t> activeIndicies;
		std::lock_guard<std::mutex> lock(mutex_);
		activeIndicies.reserve(activeCount_);
		for (size_t i = 0; i < activeCount_; ++i) {
			activeIndicies.push_back(who_[i]);
		}
		return activeIndicies;
	}

   private:
	std::array<Slot, N> slots_;
	std::array<uint32_t, N> who_;
	std::array<uint32_t, N> generation_{};
	size_t activeCount_ = 0;
	std::mutex mutex_;
};
