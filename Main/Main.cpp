#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

using Task = std::function<void()>;

std::deque<Task> g_tasks; // 중앙 공유 큐
std::mutex m;

std::atomic<int> g_pending_tasks = 0; // 대기 중인 작업 수
std::condition_variable g_task_done; // 작업 완료 신호

std::atomic<bool> g_stop = false; // 작업 종료 신호

std::atomic<double> g_sum = 0; // 작업 결과

void Worker()
{
    while (true) {
        Task task = nullptr;

        {
            std::lock_guard<std::mutex> lock(m);
            if (!g_tasks.empty()) {
                task = std::move(g_tasks.front());
                g_tasks.pop_front();
            }
        }

        if (task) {
            task();
            if (g_pending_tasks.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                g_task_done.notify_one();
            }
        } else {
            if (g_stop)
                break;
            std::this_thread::yield();
        }
    }
}

void Dummy_task(int id)
{
    double val = id;
    int count = (id + 1) * 100000;
    for (int i = 0; i < count; i++) {
        val = std::sin(val + i) + std::cos(val - i);
    }

    g_sum.fetch_add((double)val, std::memory_order_relaxed);
}

void RunTasks()
{
    std::vector<std::thread> workers;

    int num_threads = std::thread::hardware_concurrency();
    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(Worker);
    }

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 100; j++) {
            g_tasks.push_back([i] { Dummy_task(i); });
            g_pending_tasks.fetch_add(1, std::memory_order_acq_rel);
        }
    }

    {
        std::unique_lock<std::mutex> lock(m);
        g_task_done.wait(lock, [] { return g_pending_tasks.load(std::memory_order_acquire) == 0; });
    }

    g_stop = true;
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
        g_sum = 0.0;
        g_pending_tasks = 0;
        g_stop = false;

        {
            std::lock_guard<std::mutex> lock(m);
            g_tasks.clear();
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

        std::cout << "Sum: " << g_sum.load(std::memory_order_relaxed) << std::endl;
    }

    double avg = std::accumulate(results.begin(), results.end(), 0.0) / results.size();
    double min_val = *std::min_element(results.begin(), results.end());

    std::cout << "\n=== Final Benchmark Result ===" << std::endl;
    std::cout << "Average: " << avg << "ms" << std::endl;
    std::cout << "Minimum: " << min_val << "ms" << std::endl;
}
