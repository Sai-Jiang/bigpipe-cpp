#pragma once

#include <mutex>
#include <chrono>
#include <cmath>
#include <thread>
#include <algorithm>

class TokenBucket {
public:
    TokenBucket(double rate) : lastFillTime(std::chrono::steady_clock::now()), tokenPerMicroSecond(rate / 1e6),
                                tokenCount(rate), capacity(rate) {
    }

public:
    void GetToken(int count) {
        std::lock_guard<std::mutex> lock(mutex);

        tryFillBucket();

        if (tokenCount >= count) {
            tokenCount -= count;
            return;
        }

        int waittime = int(std::abs(count - tokenCount / tokenPerMicroSecond));
        std::this_thread::sleep_for(std::chrono::milliseconds(waittime));
        tokenCount = 0;
    }

private:
    void tryFillBucket() {
        auto now = std::chrono::steady_clock::now();
        auto passedTime = now - lastFillTime;
        auto passedMs = std::chrono::duration_cast<std::chrono::microseconds>(passedTime).count();
        tokenCount = std::max(passedMs * tokenPerMicroSecond + tokenCount, capacity);
        lastFillTime = now;
    }

private:
    std::chrono::steady_clock::time_point lastFillTime;	// 上一次填充时间
    double tokenPerMicroSecond;	 	                    // 每us填充的令牌个数
    double tokenCount;	                                // 剩余令牌数量
    double capacity;                                    // 桶容量
    std::mutex mutex;	                                // 线程安全
};