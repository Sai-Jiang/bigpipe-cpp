#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include "BoundedBlockingQueue.hpp"

class ThreadPool {
public:
    using Task = std::function<void()>;

    ThreadPool(int maxTasks, int maxThreads) :
                IsRunning(false),
                maxThreads(maxThreads),
                queue_(maxTasks) {
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool &operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        if (IsRunning) Stop();
    }

    bool Start();
    bool Stop();

    void PutTask(const Task &task) {
        queue_.put(task);
    }

    bool IsStarted() const { return IsRunning; }

    size_t PendingTasks() const { return queue_.size(); }

private:
    void RunInLoop();

private:
    std::atomic<bool> IsRunning;
    int maxThreads;
    std::vector<std::thread> threads_;
    BoundedBlockingQueue<Task> queue_;
};

void ThreadPool::RunInLoop() {
    while (IsRunning) {
        Task todo = queue_.take();
        if (todo) todo();
    }
}

bool ThreadPool::Start() {
    IsRunning = true;
    for (int i = 0; i < maxThreads; i++)
        threads_.emplace_back(std::thread(std::bind(&ThreadPool::RunInLoop, this)));
    return true;
}

bool ThreadPool::Stop() {
    IsRunning = false;
    for (auto &thread : threads_) {
        queue_.put(Task());
        thread.join();
    }
    return true;
}