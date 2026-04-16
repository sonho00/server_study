#pragma once

#include "SchedulerContext.hpp"

struct LocalQueue {
    constexpr static size_t CAPACITY = 1024;
    Task tasks[CAPACITY];
    size_t head = 0;
    size_t tail = 0;

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

    size_t try_pop_global()
    {
        // 글로벌 큐에서 작업 가져오기
        std::unique_lock<std::mutex> lock(ctx.m, std::try_to_lock);
        if (!lock.owns_lock() || ctx.global_queue.empty())
            return 0;

        int cnt = std::min(ctx.global_queue.size() / ctx.num_threads + 1, CAPACITY / 2);
        if (cnt <= 0)
            return 0;

        std::unique_lock<std::mutex> local_lock(mtx, std::try_to_lock);
        if (!local_lock.owns_lock())
            return 0;

        for (int i = 0; i < cnt && !ctx.global_queue.empty(); i++) {
            tasks[tail % CAPACITY] = std::move(ctx.global_queue.front());
            tail++;
            ctx.global_queue.pop_front();
        }
        return cnt;
    }

    bool try_steal(LocalQueue& victim, Task& task)
    {
        std::unique_lock<std::mutex> lock(victim.mtx, std::try_to_lock);
        if (!lock.owns_lock() || victim.head == victim.tail)
            return false;

        std::unique_lock<std::mutex> local_lock(mtx, std::try_to_lock);
        if (!local_lock.owns_lock())
            return false;

        task = std::move(victim.tasks[victim.head % CAPACITY]);
        victim.head++;
        return true;
    };
};
std::vector<std::unique_ptr<LocalQueue>> local_queues;

bool Steal(LocalQueue& my_q, int id, Task& task)
{
    static thread_local std::mt19937 rng(std::random_device { }());
    std::uniform_int_distribution<int> dist(0, ctx.num_threads - 1);

    for (int i = 0; i < local_queues.size(); i++) {
        int victim = dist(rng);
        if (victim == id)
            continue;

        if (my_q.try_steal(*local_queues[victim], task)) {
            my_q.push(std::move(task));
            return true;
        }
    }
    return false;
}

void Worker2(int id)
{
    while (true) {
        Task task = nullptr;
        {
            std::lock_guard<std::mutex> lock(ctx.m);
            if (!ctx.global_queue.empty()) {
                task = std::move(ctx.global_queue.front());
                ctx.global_queue.pop_front();
            }
        }

        if (task) {
            task();
            if (ctx.pending_tasks.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                ctx.task_done.notify_one();
            }
        } else {
            if (ctx.stop)
                break;

            if(Steal(*local_queues[id], id, task)) {
                task();
                if (ctx.pending_tasks.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    ctx.task_done.notify_one();
                }
            } else {
                std::this_thread::yield();
            }
        }
    }
}
