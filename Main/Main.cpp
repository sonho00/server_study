#include <atomic>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using Task = std::function<void()>;

std::deque<Task> g_tasks; // 중앙 공유 큐
std::mutex m;
std::atomic<bool> g_stop = false; // 작업 완료 신호
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
        }
    }

    g_stop = true;
    for (auto& t : workers) {
        t.join();
    }

    std::cout << "Result: " << g_sum << std::endl;

    return 0;
}
