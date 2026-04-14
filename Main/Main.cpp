#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using Task = std::function<void()>;

std::deque<Task> g_tasks; // 중앙 공유 큐
std::mutex m;

std::atomic<int> g_pending_tasks = 0; // 대기 중인 작업 수
std::condition_variable g_task_done; // 작업 완료 신호

std::atomic<bool> g_stop = false; // 작업 종료 신호

std::atomic<int> g_sum = 0; // 작업 결과

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

void Add()
{
    for (int i = 0; i < 100000; i++) {
        g_sum++;
    }
}

int main()
{
    std::vector<std::thread> workers;

    for (int i = 0; i < 10; i++) {
        workers.push_back(std::thread(Worker));
    }

    for (int i = 0; i < 10; i++) {
        {
            std::lock_guard<std::mutex> lock(m);
            g_tasks.push_back(Add);
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

    std::cout << "Result: " << g_sum << std::endl;

    return 0;
}
