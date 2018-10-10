#pragma once

#include <mutex>
#include <queue>
#include <cstdlib>
#include <algorithm>
#include <utility>
#include <glog/logging.h>
#include "healthStats.hpp"

struct CircuitBreakerConf {
    int BreakPeriod;
    int RecoverPeriod;
    int WinSize;
    int MinStats;
    double HealthRate;
};

class CircuitBreaker {
public:
    CircuitBreaker(CircuitBreakerConf *pconf) :
                    healthStats(pconf->WinSize, pconf->MinStats, pconf->HealthRate),
                    status(CIRCUIT_NORMAL),
                    breakTime(0),
                    breakPeriod(pconf->BreakPeriod),
                    recoverPeriod(pconf->RecoverPeriod) {

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