#include "Worker.hpp"
#include "SchedulerContext.hpp"
#include <algorithm>
#include <random>
#include <thread>

extern SchedulerContext ctx;
extern std::vector<Worker> workers;

size_t Worker::try_fetch_global(size_t batch_size = 256)
{
    std::unique_lock<std::mutex> lock(ctx.mtx, std::try_to_lock);
    if (!lock.owns_lock() || ctx.global_queue.empty())
        return 0;

    std::unique_lock<std::mutex> local_lock(mtx, std::try_to_lock);
    if (!local_lock.owns_lock())
        return 0;

    size_t cnt = std::min({ ctx.global_queue.size() / ctx.num_threads + 1, MAX_QUEUE_SIZE - (queue_tail - queue_head), batch_size });
    for (size_t i = 0; i < cnt; ++i) {
        queue[queue_tail % MAX_QUEUE_SIZE] = std::move(ctx.global_queue.front());
        queue_tail++;
        ctx.global_queue.pop_front();
    }

    return cnt;
}

size_t Worker::fill_buffer(size_t batch_size = 64)
{
    std::unique_lock<std::mutex> lock(mtx);
    size_t cnt = std::min(queue_tail - queue_head, batch_size);
    for (size_t i = 0; i < cnt; ++i) {
        buffer[buffer_tail % MAX_BUFFER_SIZE] = std::move(queue[queue_head % MAX_QUEUE_SIZE]);
        buffer_tail++;
        queue_head++;
    }
    return cnt;
}

size_t Worker::try_steal_from(Worker& victim, size_t batch_size = 256)
{
    if (victim.queue_tail - victim.queue_head == 0)
        return 0;

    std::unique_lock<std::mutex> lock(victim.mtx, std::try_to_lock);
    if (!lock.owns_lock() || victim.queue_tail - victim.queue_head == 0)
        return 0;

    std::unique_lock<std::mutex> local_lock(mtx, std::try_to_lock);
    if (!local_lock.owns_lock())
        return 0;

    size_t cnt = std::min({ (victim.queue_tail - victim.queue_head + 1) / 2, MAX_QUEUE_SIZE - (queue_tail - queue_head), batch_size });
    for (size_t i = 0; i < cnt; ++i) {
        queue[queue_tail % MAX_QUEUE_SIZE] = std::move(victim.queue[victim.queue_head % MAX_QUEUE_SIZE]);
        queue_tail++;
        victim.queue_head++;
    }

    return cnt;
}

size_t Worker::try_steal_any(size_t batch_size = 256)
{
    static thread_local std::mt19937 rng(std::random_device { }());
    std::uniform_int_distribution<int> dist(0, ctx.num_threads - 1);

    for (int i = 0; i < ctx.num_threads; i++) {
        int victim_id = dist(rng);
        int cnt = try_steal_from(workers[victim_id], batch_size);
        if (cnt > 0)
            return cnt;
    }
    return 0;
}

Task* Worker::get_next_task()
{
    if (buffer_tail - buffer_head > 0) {
        Task* task = &buffer[buffer_head % MAX_BUFFER_SIZE];
        buffer_head++;
        return task;
    }
    return nullptr;
}

void Worker::run(bool use_complex)
{
    if (use_complex) {
        run_complex();
    } else {
        run_simple();
    }
}

void Worker::run_complex()
{
    while (true) {
        Task* task = get_next_task();
        if (task) {
            (*task)();
            task_did++;
            continue;
        }

        if (fill_buffer() > 0 || try_fetch_global() > 0 || try_steal_any() > 0) {
            continue;
        }

        if (ctx.stop)
            break;

        if (task_did > 0) {
            if (ctx.pending_tasks.fetch_sub(task_did, std::memory_order_acq_rel) == task_did) {
                ctx.task_done.notify_one();
            }
            task_did = 0;
        }

        std::this_thread::yield();
    }
}

void Worker::run_simple()
{
    while (true) {
        Task task = nullptr;
        {
            std::lock_guard<std::mutex> lock(ctx.mtx);
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

void Worker::Dummy_task(int count)
{
    double val = 0.0;
    for (int i = 0; i < count; i++) {
        val = std::sin(val + i) + std::cos(val - i);
    }
    this->sum += val;
}
