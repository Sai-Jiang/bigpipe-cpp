#pragma once

#include <sstream>
#include <cpprest/json.h>
#include <cpprest/http_client.h>
#include <glog/logging.h>
#include "rate_limit.h"
#include "circuit_breaker.hpp"
#include "../proto/reqmsg.hpp"
#include "../base/ThreadPool.hpp"

using namespace web;
using namespace web::http;
using namespace web::http::client;

class AsyncClient {
public:
    AsyncClient(int nWorkers, int retries, int timeout) : workers(new ThreadPool(20, nWorkers)),
                                                          retries(retries),
                                                          timeout(timeout) {
    }

    bool Start() { workers->Start(); }
    bool Stop() { workers->Stop(); }

    bool SetTokenBucket(double rate);
    bool SetCircuitBreaker(const CircuitBreakerConf &conf);

public:
    void AsyncCall(const RequestMessage &msg);
    void Call(const RequestMessage &msg);

private:
    void CallWithRetry(const RequestMessage &msg);
    void notifyCircuitBreaker(bool success);

private:
    int retries;
    int timeout;
    std::unique_ptr<TokenBucket> rateLimit;
    std::unique_ptr<CircuitBreaker> circuitBreaker;
    std::unique_ptr<ThreadPool> workers;
};

bool AsyncClient::SetTokenBucket(double rate) {
    rateLimit = std::make_unique<TokenBucket>(rate);
    return true;
}

bool AsyncClient::SetCircuitBreaker(const CircuitBreakerConf &conf) {
    circuitBreaker = std::make_unique<CircuitBreaker>(conf);
    return true;
}

void AsyncClient::CallWithRetry(const RequestMessage &msg) {
    bool success = false;

    for (int i = 0; retries + 1; i++) {
        http_client client(msg.GetURL());

        http_request request(methods::POST);
        for (auto &head : msg.GetHTTPHeaders())
            request.headers().add(head.first, head.second);
        request.headers().add("Content-Type", "application/octet-stream");
        request.headers().add("Content-Length", std::to_string(msg.GetData().size()));

        auto reqStartTime = std::chrono::steady_clock::now();
        http_response resp = client.request(request).get();
        auto reqUsedTime = std::chrono::steady_clock::now() - reqStartTime;
        auto reqUsedTimeMS = std::chrono::duration_cast<std::chrono::microseconds>(reqUsedTime).count();

        resp.body().close();

        if (resp.status_code() != 200) {
            notifyCircuitBreaker(false);
            std::ostringstream oss;
            oss << "HTTP调用失败(%" << i << ") (" << reqUsedTimeMS << "ms) " << resp.status_code() << std::endl;
            LOG(WARNING) << oss.str();
            continue;
        }

        success = true;
        notifyCircuitBreaker(true);
        std::ostringstream oss;
        oss << "HTTP调用成功(%" << i << ") (" << reqUsedTimeMS << "ms) " << resp.status_code() << std::endl;
        LOG(INFO) << oss.str();
        break;
    }
}

void AsyncClient::Call(const RequestMessage &msg) {
    if (circuitBreaker) {
        while (circuitBreaker->IsBreak())
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    assert(rateLimit != nullptr);
    rateLimit->GetToken(1);

    CallWithRetry(msg);
}

void AsyncClient::AsyncCall(const RequestMessage &msg) {
    assert(workers->IsStarted());
    workers->PutTask([&](){ Call(msg); });
}

void AsyncClient::notifyCircuitBreaker(bool success) {
    if (circuitBreaker) {
        if (success) {
            circuitBreaker->OnSuccess();
        } else {
            circuitBreaker->OnFail();
        }
    }
}