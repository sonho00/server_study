#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

using Task = std::function<void()>;

struct SchedulerContext {
	int num_threads_ = std::thread::hardware_concurrency();

	std::deque<Task*> global_queue_;
	std::mutex mtx_;

	std::atomic<int> pending_tasks_;
	std::atomic<bool> stop_;
	std::condition_variable task_done_;
};

extern SchedulerContext ctx;
