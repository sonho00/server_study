#include "SchedulerContext.hpp"
#include "Worker.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>

SchedulerContext ctx;

// false: 단순 글로벌 큐 워커, true: 로컬 큐 + 워크 스틸링
bool use_complex_worker = true;
std::vector<Worker> workers(ctx.num_threads);

int MakeTask()
{
    std::lock_guard<std::mutex> lock(ctx.mtx);
    for (int i = 0; i < ctx.num_threads; ++i) {
        for (int j = 0; j < 100000; ++j) {
            ctx.global_queue.push_back([i] {
                workers[i].Dummy_task(i * 50 + 250);
            });
        }
    }
    return ctx.num_threads * 100000;
}

void RunTasks()
{
    ctx.pending_tasks = MakeTask();

    std::vector<std::thread> threads;
    for (int i = 0; i < ctx.num_threads; i++) {
        threads.emplace_back([i] { workers[i].run(use_complex_worker); });
    }

    {
        std::unique_lock<std::mutex> lock(ctx.mtx);
        ctx.task_done.wait(lock, [] { return ctx.pending_tasks.load(std::memory_order_acquire) == 0; });
    }
    ctx.stop = true;

    for (auto& t : threads) {
        t.join();
    }
}

int main()
{
    std::cout << std::fixed << std::setprecision(3);

    int iterations = 11; // 워밍업 1회 + 측정 10회
    std::vector<double> elapsedTimes;

    for (int i = 0; i < iterations; i++) {
        ctx.stop = false;
        for (auto& w : workers) {
            w.queue_head = w.queue_tail = 0;
            w.buffer_head = w.buffer_tail = 0;
            w.task_did = 0;
            w.sum = 0.0;
        }

        {
            std::lock_guard<std::mutex> lock(ctx.mtx);
            ctx.global_queue.clear();
        }

        auto start = std::chrono::high_resolution_clock::now();
        RunTasks();
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        if (i == 0) {
            std::cout << "Warm-up: " << elapsed.count() << "ms (Discarded) ";
        } else {
            elapsedTimes.push_back(elapsed.count());
            std::cout << "Iteration " << i << ": " << elapsed.count() << "ms ";
        }

        double total_sum = 0.0;
        for (const auto& w : workers) {
            total_sum += w.sum.load(std::memory_order_relaxed);
        }
        std::cout << "- Total Sum: " << total_sum << std::endl;
    }

    double avg = std::accumulate(elapsedTimes.begin(), elapsedTimes.end(), 0.0) / elapsedTimes.size();
    double min_val = *std::min_element(elapsedTimes.begin(), elapsedTimes.end());

    std::cout << "\n=== Final Benchmark Result ===" << std::endl;
    std::cout << "Average: " << avg << "ms" << std::endl;
    std::cout << "Minimum: " << min_val << "ms" << std::endl;
}
