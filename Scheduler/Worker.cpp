#include "Worker.hpp"

#include <algorithm>
#include <random>
#include <thread>

#include "SchedulerContext.hpp"

extern SchedulerContext ctx;
extern std::vector<Worker> workers;

size_t Worker::TryFetchGlobal(size_t batch_size = 256) {
	std::unique_lock<std::mutex> lock(ctx.mtx_, std::try_to_lock);
	if (!lock.owns_lock() || ctx.global_queue_.empty()) return 0;

	std::unique_lock<std::mutex> local_lock(mtx_, std::try_to_lock);
	if (!local_lock.owns_lock()) return 0;

	size_t cnt =
		std::min({ctx.global_queue_.size() / ctx.num_threads_ + 1,
				  kQueueSize_ - (queue_tail_ - queue_head_), batch_size});
	for (size_t i = 0; i < cnt; ++i) {
		queue_[queue_tail_ % kQueueSize_] = ctx.global_queue_.front();
		queue_tail_++;
		ctx.global_queue_.pop_front();
	}

	return cnt;
}

size_t Worker::TryStealFrom(Worker& victim, size_t batch_size = 256) {
	if (victim.queue_tail_ - victim.queue_head_ == 0) return 0;

	std::unique_lock<std::mutex> lock(victim.mtx_, std::try_to_lock);
	if (!lock.owns_lock() || victim.queue_tail_ - victim.queue_head_ == 0)
		return 0;

	std::unique_lock<std::mutex> local_lock(mtx_, std::try_to_lock);
	if (!local_lock.owns_lock()) return 0;

	size_t cnt =
		std::min({(victim.queue_tail_ - victim.queue_head_ + 1) / 2,
				  kQueueSize_ - (queue_tail_ - queue_head_), batch_size});
	for (size_t i = 0; i < cnt; ++i) {
		queue_[queue_tail_ % kQueueSize_] =
			victim.queue_[victim.queue_head_ % kQueueSize_];
		queue_tail_++;
		victim.queue_head_++;
	}

	return cnt;
}

size_t Worker::TryStealAny(size_t batch_size = 256) {
	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, ctx.num_threads_ - 1);

	for (int i = 0; i < ctx.num_threads_; i++) {
		int victim_id = dist(rng);
		int cnt = TryStealFrom(workers[victim_id], batch_size);
		if (cnt > 0) return cnt;
	}
	return 0;
}

void Worker::Run(bool use_complex) {
	if (use_complex) {
		RunComplex();
	} else {
		RunSimple();
	}
}

void Worker::RunComplex() {
	while (true) {
		Task* task = GetNextTask();
		if (task) {
			(*task)();
			task_did_++;
			continue;
		}

		if (FillBuffer() > 0 || TryFetchGlobal() > 0 || TryStealAny() > 0) {
			continue;
		}

		if (ctx.stop_) break;

		if (task_did_ > 0) {
			if (ctx.pending_tasks_.fetch_sub(
					task_did_, std::memory_order_acq_rel) == task_did_) {
				ctx.task_done_.notify_one();
			}
			task_did_ = 0;
		}

		std::this_thread::yield();
	}
}

void Worker::RunSimple() {
	while (true) {
		Task* task = nullptr;
		{
			std::lock_guard<std::mutex> lock(ctx.mtx_);
			if (!ctx.global_queue_.empty()) {
				task = ctx.global_queue_.front();
				ctx.global_queue_.pop_front();
			}
		}

		if (task) {
			(*task)();
			if (ctx.pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) ==
				1) {
				ctx.task_done_.notify_one();
			}
		} else {
			if (ctx.stop_) break;
			std::this_thread::yield();
		}
	}
}

void Worker::DummyTask(int count) {
	double val = 0.0;
	for (int i = 0; i < count; i++) {
		val = std::sin(val + i) + std::cos(val - i);
	}
	this->sum_ += val;
}
