#pragma once

#include <mutex>
#include <queue>
#include <algorithm>
#include <utility>

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

    void Success() {
        UpdateBuckets();
        buckets.back().nSuccess++;
    }

    void Fail() {
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
    std::mutex mutex;
private:
    enum CircuitStatus {
        CIRCUIT_NONE,
        CIRCUIT_NORMAL,
        CIRCUIT_BREAK,
        CIRCUIT_RECOVER
    };
};