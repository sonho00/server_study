#pragma once

#include "SchedulerContext.hpp"

struct alignas(64) Worker {
    static constexpr size_t kQueueSize_ = 512;
    static constexpr size_t kMaxBufferSize_ = 64;

    Task queue_[kQueueSize_];
    size_t queue_head_ = 0;
    size_t queue_tail_ = 0;

    Task buffer_[kMaxBufferSize_];
    size_t buffer_head_ = 0;
    size_t buffer_tail_ = 0;

    size_t task_did_ = 0;
    std::atomic<double> sum_ = 0.0;

    std::mutex mtx_;

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
