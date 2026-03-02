#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> task_queue;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop_flag;
    std::atomic<int> active_threads;
    int num_threads;

public:
    ThreadPool(int threads = std::thread::hardware_concurrency())
        : stop_flag(false), active_threads(0), num_threads(threads) {

        std::cout << "[THREAD POOL] Starting with " << threads << " threads" << std::endl;

        for (int i = 0; i < threads; ++i) {
            workers.emplace_back([this, i] {
                std::cout << "[WORKER-" << i << "] Started" << std::endl;
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop_flag.load() || !task_queue.empty();
                        });
                        if (stop_flag.load() && task_queue.empty()) {
                            std::cout << "[WORKER-" << i << "] Shutting down" << std::endl;
                            return;
                        }
                        if (!task_queue.empty()) {
                            task = std::move(task_queue.front());
                            task_queue.pop();
                        }
                    }
                    if (task) {
                        active_threads++;
                        task();
                        active_threads--;
                    }
                }
            });
        }
        std::cout << "[THREAD POOL] All workers ready" << std::endl;
    }

    // Template defined HERE in header (not in .cpp) - this fixes your error
    template<class F>
    void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            task_queue.emplace(std::forward<F>(task));
            std::cout << "[THREAD POOL] Task enqueued. Queue size: "
                      << task_queue.size() << std::endl;
        }
        condition.notify_one();
    }

    void shutdown() {
        stop_flag = true;
        condition.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
        std::cout << "[THREAD POOL] Shutdown complete" << std::endl;
    }

    int getActiveThreadCount() const { return active_threads.load(); }
    int getTotalThreads() const { return num_threads; }
    int getQueueSize() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return task_queue.size();
    }

    ~ThreadPool() { shutdown(); }
};

#endif
