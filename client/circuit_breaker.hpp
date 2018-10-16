#pragma once

#include <mutex>
#include <queue>
#include <cstdlib>
#include <algorithm>
#include <utility>
#include <glog/logging.h>

struct CircuitBreakerConf {
    int BreakPeriod;
    int RecoverPeriod;
    int WinSize;
    int MinStats;
    double HealthRate;
};

class HealthStats {
public:
    HealthStats(int maxBuckets, int bucketThresh, double healthThresh) :
            maxBuckets(maxBuckets),
            bucketThresh(bucketThresh),
            healthThresh(healthThresh) {
        updateTime = std::time(nullptr);
    }

public:
    std::pair<bool, double> IsHealthy() {
        UpdateBuckets();

        long TotalSuccess = 0, TotalFail = 0, Total = 0;
        for (auto &bucket : buckets) {
            TotalSuccess += bucket.nSuccess;
            TotalFail += bucket.nFail;
        }
        Total = TotalSuccess + TotalFail;
        if (Total == 0) {
            return {true, 1};
        }

        double rate = (double)TotalSuccess / (double)Total;

        return (Total < bucketThresh || rate >= healthThresh) ?
               std::make_pair(true, rate) : std::make_pair(false, rate);
    }

    void UpdateBuckets() {
        std::time_t now = std::time(nullptr);
        long bucketsToClean = (long)now - updateTime;
        if (bucketsToClean <= 0) {
            return;
        } else if (bucketsToClean >= buckets.size()) {
            buckets.erase(buckets.begin(), buckets.end());
        } else {
            buckets.erase(buckets.begin(), buckets.begin() + bucketsToClean);
        }
        updateTime = now;
    };

    void OnSuccess() {
        UpdateBuckets();
        buckets.back().nSuccess++;
    }

    void OnFail() {
        UpdateBuckets();
        buckets.back().nFail++;
    }

private:
    struct StatsBucket {
        int nSuccess, nFail;
    };

private:
    int maxBuckets;
    std::deque<StatsBucket> buckets;        // 滑动窗口, 每秒一个桶
    std::time_t updateTime;                 // 当前窗口末尾的秒级unix时间戳
    int bucketThresh;                       // 少于该打点数量直接返回健康
    double healthThresh;
};

class CircuitBreaker {
public:
    CircuitBreaker(const CircuitBreakerConf &conf) :
                    healthStats(conf.WinSize, conf.MinStats, conf.HealthRate),
                    status(CIRCUIT_NORMAL),
                    breakTime(0),
                    breakPeriod(conf.BreakPeriod),
                    recoverPeriod(conf.RecoverPeriod) {

    }

public:
    bool IsBreak() {
        std::lock_guard<std::mutex> lock(mutex);

        auto now = std::time(nullptr);

        auto health = healthStats.IsHealthy();

        bool isBreak = false;
        switch (status) {
            case CIRCUIT_NORMAL:
                if (!health.first) {
                    status = CIRCUIT_BREAK;
                    breakTime = now;
                    isBreak = true;
                }
                break;
            case CIRCUIT_BREAK:
                if (now - breakTime < breakPeriod || !health.first) {
                    isBreak = true;
                } else {
                    status = CIRCUIT_RECOVER;
                }
                break;
            case CIRCUIT_RECOVER:
                if (!health.first) {
                    status = CIRCUIT_BREAK;
                    breakTime = now;
                    isBreak = true;
                } else {
                    if (now - breakTime >= breakPeriod + recoverPeriod) {
                        status = CIRCUIT_NORMAL;
                    } else {
                        double passPercent = (double) (now - breakTime) / double(breakPeriod + recoverPeriod);
                        if (passPercent * 100 < rand() % 100) {
                            isBreak = true;
                        }
                    }
                }
                break;
            default:
                LOG(ERROR) << "CircuitBreaker Fatal Error: Wrong Status" << std::endl;
        }

        return isBreak;
    }

    void OnSuccess() {
        std::lock_guard<std::mutex> lock(mutex);
        healthStats.OnSuccess();
    }

    void OnFail() {
        std::lock_guard<std::mutex> lock(mutex);
        healthStats.OnFail();
    }

private:
    enum CircuitStatus {
        CIRCUIT_NONE,
        CIRCUIT_NORMAL,
        CIRCUIT_BREAK,
        CIRCUIT_RECOVER
    };

private:
    std::mutex mutex;
    HealthStats healthStats;    // 健康统计
    CircuitStatus status;       // 熔断状态
    std::time_t breakTime;      // 熔断的时间点(秒)
    int breakPeriod;            // 熔断封锁时间
    int recoverPeriod;          // 熔断恢复时间
};