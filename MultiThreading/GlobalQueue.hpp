#pragma once

#include "SchedulerContext.hpp"

extern SchedulerContext ctx;

void Worker1(int id)
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
            std::this_thread::yield();
        }
    }
}
