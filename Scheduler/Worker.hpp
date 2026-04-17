#pragma once

#include "SchedulerContext.hpp"

struct alignas(64) Worker {
    static constexpr size_t MAX_QUEUE_SIZE = 512;
    static constexpr size_t MAX_BUFFER_SIZE = 64;

    Task queue[MAX_QUEUE_SIZE];
    size_t queue_head = 0;
    size_t queue_tail = 0;

    Task buffer[MAX_BUFFER_SIZE];
    size_t buffer_head = 0;
    size_t buffer_tail = 0;

    size_t task_did = 0;
    std::atomic<double> sum = 0.0;

    std::mutex mtx;
    size_t try_fetch_global(size_t batch_size);
    size_t fill_buffer(size_t batch_size);
    size_t try_steal_from(Worker& victim, size_t batch_size);
    size_t try_steal_any(size_t batch_size);
    Task* get_next_task();

    void Dummy_task(int count);
    void run(bool use_complex);
    void run_complex();
    void run_simple();
    void SimpleWorker();
};
