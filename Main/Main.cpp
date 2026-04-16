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
#include <random>
#include <thread>
#include <vector>

using Task = std::function<void()>;

struct WorkerQueue {
    static constexpr size_t CAPACITY = 1024;
    Task tasks[CAPACITY];
    std::atomic<size_t> head { 0 };
    std::atomic<size_t> tail { 0 };
    std::mutex mtx;

    void push(Task task)
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks[tail % CAPACITY] = std::move(task);
        tail++;
    }

    bool try_pop(Task& task)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (head == tail)
            return false;
        tail--;
        task = std::move(tasks[tail % CAPACITY]);
        return true;
    }

    bool try_steal(Task& task)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (head == tail)
            return false;
        task = std::move(tasks[head % CAPACITY]);
        head++;
        return true;
    }
};

std::vector<Task> g_global_queue;
std::vector<std::unique_ptr<WorkerQueue>> g_local_queues;
std::mutex g_m;

std::atomic<int> g_pending_tasks = 0; // 대기 중인 작업 수
std::condition_variable g_task_done; // 작업 완료 신호

std::atomic<bool> g_stop = false; // 작업 종료 신호

std::atomic<double> g_sum = 0; // 작업 결과

bool Steal(int id, WorkerQueue& my_q, Task& task)
{
    static thread_local std::mt19937 gen(std::random_device { }());
    std::uniform_int_distribution<int> dis(0, g_local_queues.size() - 1);

    for (int i = 0; i < g_local_queues.size(); i++) {
        int victim_id = dis(gen);
        if (victim_id == id)
            continue;

        if (g_local_queues[victim_id]->try_steal(task)) {
            my_q.push(std::move(task));
            return true;
        }
    }
    return false;
}

bool Try_pop_global(WorkerQueue& my_q, Task& task)
{
    // 중앙 큐에서 작업 가져오기
    std::lock_guard<std::mutex> lock(g_m);
    int cnt = std::min(g_global_queue.size(), my_q.CAPACITY / 2);
    if (cnt <= 0)
        return false;

    for (int i = 0; i < cnt && !g_global_queue.empty(); i++) {
        my_q.push(std::move(g_global_queue.back()));
        g_global_queue.pop_back();
    }
    return true;
}

void Worker(int id)
{
    auto& my_q = *g_local_queues[id];
    while (true) {
        Task task = nullptr;
        if (my_q.try_pop(task)) {
            task();
            if (g_pending_tasks.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                g_task_done.notify_one();
            }
            continue;
        }

        if (Steal(id, my_q, task) || Try_pop_global(my_q, task))
            continue;

        if (g_stop)
            break;

        std::this_thread::yield();
    }
}

void Dummy_task(int id)
{
    double val = id;
    int count = (id + 5) * 50;
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
        g_local_queues.push_back(std::make_unique<WorkerQueue>());
        workers.emplace_back(Worker, i);
    }

    {
        std::lock_guard<std::mutex> lock(g_m);
        g_pending_tasks = num_threads * 100000;
        for (int i = 0; i < num_threads; ++i) {
            for (int j = 0; j < 100000; ++j) {
                g_global_queue.push_back([i] { Dummy_task(i); });
            }
        }
    }

    {
        std::unique_lock<std::mutex> lock(g_m);
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
        g_local_queues.clear();

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
