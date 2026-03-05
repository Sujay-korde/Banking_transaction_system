#ifndef THREADPOOL_H
#define THREADPOOL_H

/*
 * ThreadPool — Week 1 + Week 2 + Week 3
 *
 * OS Concepts:
 *   THREADS           — pool of worker threads
 *   MUTEX             — protects task queue
 *   CONDITION VARIABLE— workers sleep when idle, wake on new task
 *   PROCESS SCHEDULING— FIFO distribution across workers
 */

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>
#include <chrono>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex q_mutex;
    std::condition_variable cv;
    std::atomic<bool> stop{false};
    std::atomic<int>  active{0};
    int num_threads;

public:
    ThreadPool(int n = std::thread::hardware_concurrency()) : num_threads(n) {
        for (int i = 0; i < n; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(q_mutex);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    active++;
                    task();
                    active--;
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(q_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        cv.notify_one();
    }

    void waitAll() {
        while (true) {
            std::unique_lock<std::mutex> lock(q_mutex);
            if (tasks.empty() && active == 0) break;
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    int getActive()  const { return active.load(); }
    int getThreads() const { return num_threads; }

    int getQueueSize() {
        std::lock_guard<std::mutex> lock(q_mutex);
        return tasks.size();
    }

    ~ThreadPool() {
        stop = true;
        cv.notify_all();
        for (auto& w : workers)
            if (w.joinable()) w.join();
    }
};

#endif
