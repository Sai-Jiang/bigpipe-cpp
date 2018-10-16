#pragma once

#include <csignal>
#include <glog/logging.h>
#include <mutex>
#include <condition_variable>

class SignalHandler {
public:
    SignalHandler() = delete;

    static void hookSIGINT() {
        ::signal(SIGINT, handleUserInterrupt);
    }

    static void waitForUserInterrupt() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock);
        LOG(INFO) << "User has signaled to interrupt program..." << std::endl;
    }

private:
    static void handleUserInterrupt(int signo) {
        if (signo == SIGINT) {
            LOG(INFO) << "SIGINT trapped ..." << std::endl;
            cond_.notify_one();
        }
    }

private:
    static std::mutex mutex_;
    static std::condition_variable cond_;
};