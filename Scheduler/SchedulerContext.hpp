#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <random>

using Task = std::function<void()>;

struct SchedulerContext {
    int num_threads = std::thread::hardware_concurrency();

    std::deque<Task> global_queue;
    std::mutex mtx;

    std::atomic<int> pending_tasks;
    std::atomic<bool> stop;
    std::condition_variable task_done;
};

extern SchedulerContext ctx;
