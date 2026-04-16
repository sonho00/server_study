#include "GlobalQueue.hpp"
#include "LocalQueue.hpp"
#include "SchedulerContext.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>

extern void Worker1(int id);
extern void Worker2(int id);

auto SelectedWorker = Worker2;

void Dummy_task(int id)
{
    double val = id;
    int count = (id + 5) * 50;
    for (int i = 0; i < count; i++) {
        val = std::sin(val + i) + std::cos(val - i);
    }

    ctx.sum.fetch_add((double)val, std::memory_order_relaxed);
}

int MakeTask()
{
    std::lock_guard<std::mutex> lock(ctx.m);
    for (int i = 0; i < ctx.num_threads; ++i) {
        for (int j = 0; j < 100000; ++j) {
            ctx.global_queue.push_back([i] { Dummy_task(i); });
        }
    }
    return ctx.num_threads * 100000;
}

void RunTasks()
{
    local_queues.clear();
    local_queues.reserve(ctx.num_threads);
    for (int i = 0; i < ctx.num_threads; i++) {
        local_queues.emplace_back(std::make_unique<Worker>());
    }

    std::vector<std::thread> workers(ctx.num_threads);
    for (int i = 0; i < ctx.num_threads; i++) {
        workers[i] = std::thread(SelectedWorker, i);
    }

    ctx.pending_tasks = MakeTask();

    {
        std::unique_lock<std::mutex> lock(ctx.m);
        ctx.task_done.wait(lock, [] { return ctx.pending_tasks.load(std::memory_order_acquire) == 0; });
    }
    ctx.stop = true;

    for (auto& t : workers) {
        t.join();
    }
}

int main()
{
    std::cout << std::fixed << std::setprecision(3);

    int iterations = 11; // 워밍업 1회 + 측정 10회
    std::vector<double> results;

    for (int i = 0; i < iterations; i++) {
        ctx.sum = 0.0;
        ctx.stop = false;

        {
            std::lock_guard<std::mutex> lock(ctx.m);
            ctx.global_queue.clear();
        }

        auto start = std::chrono::high_resolution_clock::now();
        RunTasks();
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        if (i == 0) {
            std::cout << "Warm-up: " << elapsed.count() << "ms (Discarded) ";
        } else {
            results.push_back(elapsed.count());
            std::cout << "Iteration " << i << ": " << elapsed.count() << "ms ";
        }

        std::cout << "Sum: " << ctx.sum.load(std::memory_order_relaxed) << std::endl;
    }

    double avg = std::accumulate(results.begin(), results.end(), 0.0) / results.size();
    double min_val = *std::min_element(results.begin(), results.end());

    std::cout << "\n=== Final Benchmark Result ===" << std::endl;
    std::cout << "Average: " << avg << "ms" << std::endl;
    std::cout << "Minimum: " << min_val << "ms" << std::endl;
}
