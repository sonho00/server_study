#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <numeric>
#include <vector>

#include "Logger.hpp"

template <size_t N, size_t stateCount = 2>
class SparseSetV2 {
   public:
	struct Slot {
		uint32_t where_ = 0;
		uint32_t generation_ = 0;
		size_t state_ = 0;
	};

	static constexpr uint64_t kInvalidHandle = static_cast<uint64_t>(-1);

	SparseSetV2();

	// idle(state 0)에서 핸들을 꺼내서 state로 이동.
	// idle에 핸들이 없거나 state가 유효하지 않으면 kInvalidHandle 반환
	[[nodiscard]] uint64_t Pop(size_t state = 1);
	// 핸들을 idle(state 0)로 이동. 핸들이 유효하지 않으면 false 반환
	bool Push(uint64_t handle);

	bool MoveToState(uint64_t handle, size_t newState);
	[[nodiscard]] std::vector<uint64_t> GetIndicesInState(
		size_t state = stateCount - 1);

	[[nodiscard]] bool IsValid(uint64_t handle) const;
	[[nodiscard]] uint64_t GetHandle(uint32_t who) const;

   private:
	bool MoveToStateInternal(uint64_t handle, size_t newState);
	bool IncrementStateInternal(uint64_t handle);
	bool DecrementStateInternal(uint64_t handle);

	static constexpr uint8_t kIndexShift = 32;

	std::array<Slot, N> sparse_;
	std::array<uint32_t, N> dense_;
	// dense_[cursor_[state]]부터 dense_[cursor_[state + 1] - 1]까지
	// state에 있는 핸들들의 인덱스
	std::array<uint32_t, stateCount + 1> cursor_;
	std::mutex mutex_;
};

template <size_t N, size_t stateCount>
SparseSetV2<N, stateCount>::SparseSetV2() {
	for (size_t i = 0; i < N; ++i) {
		sparse_[i].where_ = i;
	}
	std::iota(std::begin(dense_), std::end(dense_), 0);
	std::fill(cursor_.begin(), cursor_.end(), N);
	cursor_[0] = 0;
}

template <size_t N, size_t stateCount>
uint64_t SparseSetV2<N, stateCount>::Pop(size_t state) {
	if (state >= stateCount) {
		LOG_ERROR("Invalid state: {}, stateCount: {}", state, stateCount);
		return kInvalidHandle;
	}
	uint64_t handle;
	{
		std::lock_guard lock(mutex_);
		if (cursor_[1] < 1) {
			LOG_ERROR("No available handles.");
			return kInvalidHandle;
		}

		// state 0에서 한 개를 꺼내서 state로 이동
		handle = GetHandle(dense_[cursor_[1] - 1]);
		MoveToStateInternal(handle, state);
	}
	return handle;
}

template <size_t N, size_t stateCount>
bool SparseSetV2<N, stateCount>::Push(uint64_t handle) {
	if (!IsValid(handle)) return false;

	auto who = static_cast<uint32_t>(handle);
	{
		std::lock_guard lock(mutex_);
		MoveToStateInternal(handle, 0);
	}
	sparse_[who].generation_++;

	return true;
}

template <size_t N, size_t stateCount>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool SparseSetV2<N, stateCount>::MoveToState(uint64_t handle, size_t newState) {
	if (newState >= stateCount) {
		LOG_ERROR("Invalid newState: {}, stateCount: {}", newState, stateCount);
		return false;
	}
	if (!IsValid(handle)) {
		LOG_ERROR("Invalid handle: {}", handle);
		return false;
	}

	std::lock_guard lock(mutex_);
	return MoveToStateInternal(handle, newState);
}

template <size_t N, size_t stateCount>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool SparseSetV2<N, stateCount>::MoveToStateInternal(uint64_t handle,
													 size_t newState) {
	auto who = static_cast<uint32_t>(handle);
	while (sparse_[who].state_ < newState) {
		if (!IncrementStateInternal(handle)) return false;
	}
	while (sparse_[who].state_ > newState) {
		if (!DecrementStateInternal(handle)) return false;
	}

	return true;
}

template <size_t N, size_t stateCount>
std::vector<uint64_t> SparseSetV2<N, stateCount>::GetIndicesInState(
	size_t state) {
	std::vector<uint64_t> result;
	if (state >= stateCount) {
		LOG_ERROR("Invalid state: {}, stateCount: {}", state, stateCount);
		return result;
	}

	{
		std::lock_guard lock(mutex_);
		// cursor_[state]부터 cursor_[state + 1]까지가 state에 있는 핸들
		result.reserve(cursor_[state + 1] - cursor_[state]);
		for (size_t i = cursor_[state]; i < cursor_[state + 1]; ++i) {
			result.push_back(GetHandle(dense_[i]));
		}
	}
	return result;
}

template <size_t N, size_t stateCount>
bool SparseSetV2<N, stateCount>::IsValid(uint64_t handle) const {
	auto who = static_cast<uint32_t>(handle);
	auto generation = static_cast<uint32_t>(handle >> kIndexShift);

	bool valid = who < N && sparse_[who].generation_ == generation;
	if (!valid) {
		LOG_ERROR(
			"Invalid handle: {}, who: {}, Expected generation: {}, "
			"Actual generation: {}",
			handle, who, generation, sparse_[who].generation_);
	}
	return valid;
}

template <size_t N, size_t stateCount>
uint64_t SparseSetV2<N, stateCount>::GetHandle(uint32_t who) const {
	// 상위 32비트는 generation, 하위 32비트는 who
	return (static_cast<uint64_t>(sparse_[who].generation_) << kIndexShift) |
		   who;
}

template <size_t N, size_t stateCount>
bool SparseSetV2<N, stateCount>::IncrementStateInternal(uint64_t handle) {
	auto who = static_cast<uint32_t>(handle);
	size_t currentState = sparse_[who].state_;
	if (currentState >= stateCount - 1) {
		LOG_ERROR("Cannot increment state: {}", handle);
		return false;
	}

	size_t nextState = currentState + 1;
	size_t where = sparse_[who].where_;
	size_t swapWith = cursor_[nextState] - 1;

	std::swap(dense_[where], dense_[swapWith]);
	sparse_[dense_[where]].where_ = where;
	sparse_[dense_[swapWith]].where_ = swapWith;

	cursor_[nextState]--;
	sparse_[who].state_ = nextState;

	return true;
}

template <size_t N, size_t stateCount>
bool SparseSetV2<N, stateCount>::DecrementStateInternal(uint64_t handle) {
	auto who = static_cast<uint32_t>(handle);
	size_t currentState = sparse_[who].state_;
	if (currentState == 0) {
		LOG_ERROR("Cannot decrement state: {}", handle);
		return false;
	}

	size_t prevState = currentState - 1;
	size_t where = sparse_[who].where_;
	size_t swapWith = cursor_[currentState];

	std::swap(dense_[where], dense_[swapWith]);
	sparse_[dense_[where]].where_ = where;
	sparse_[dense_[swapWith]].where_ = swapWith;

	cursor_[currentState]++;
	sparse_[who].state_ = prevState;

	return true;
}
