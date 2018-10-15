#progma once

#include <csignal>
#include <glog/logging.h>

class SignalHandler {
public:
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
        if (signo == ::SIGINT) {
            LOG(INFO) << "SIGINT trapped ..." << std::endl;
            _cond.notify_one();
        }
    }

private:
    static std::mutex mutex_;
    static std::condition_variable cond_;
};