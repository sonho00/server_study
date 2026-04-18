#pragma once

#include "SchedulerContext.hpp"

// constexpr size_t kDestructiveSize =
// std::hardware_destructive_interference_size;
constexpr size_t kDestructiveSize = 64;

struct Worker {
	static constexpr size_t kQueueSize_ = 512;
	static constexpr size_t kMaxBufferSize_ = 64;

	alignas(kDestructiveSize) std::mutex mtx_;

	alignas(kDestructiveSize) Task* queue_[kQueueSize_];
	size_t queue_head_ = 0;
	size_t queue_tail_ = 0;

	Task* buffer_[kMaxBufferSize_];
	size_t buffer_head_ = 0;
	size_t buffer_tail_ = 0;

	size_t task_did_ = 0;
	std::atomic<double> sum_ = 0.0;

	size_t TryFetchGlobal(size_t batch_size);
	size_t FillBuffer(size_t batch_size);
	size_t TryStealFrom(Worker& victim, size_t batch_size);
	size_t TryStealAny(size_t batch_size);
	Task* GetNextTask();

	void DummyTask(int count);
	void Run(bool use_complex);
	void RunComplex();
	void RunSimple();
	void SimpleWorker();
};

inline size_t Worker::FillBuffer(size_t batch_size = 64) {
	std::unique_lock<std::mutex> lock(mtx_);
	size_t cnt = std::min(queue_tail_ - queue_head_, batch_size);
	for (size_t i = 0; i < cnt; ++i) {
		buffer_[buffer_tail_ % kMaxBufferSize_] =
			queue_[queue_head_ % kQueueSize_];
		buffer_tail_++;
		queue_head_++;
	}
	return cnt;
}

inline Task* Worker::GetNextTask() {
	if (buffer_tail_ - buffer_head_ > 0) {
		Task* task = buffer_[buffer_head_ % kMaxBufferSize_];
		buffer_head_++;
		return task;
	}
	return nullptr;
}
