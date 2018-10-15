#pragma once
#include <boost/noncopyable.hpp>
#include <mutex>
#include <condition_variable>
#include <deque>

template <typename T>
class BlockingQueue : boost::noncopyable {
public:
    BlockingQueue() : mutex_(), cond_(), queue_() {

    }

    void put(const T& x) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(x);
        cond_.notify_one();
    }

    void put(T&& x) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(x);
        cond_.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty(); });
        assert(!queue_.empty());
        T front(queue_.front());
        queue_.pop_front();
        return front;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::deque<T> queue_;
};