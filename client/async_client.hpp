#pragma once

#include <sstream>
#include <cpprest/json.h>
#include <cpprest/http_client.h>
#include <glog/logging.h>
#include "rate_limit.h"
#include "circuit_breaker.hpp"
#include "../proto/reqmsg.hpp"

using namespace web;
using namespace web::http;
using namespace web::http::client;

class AsyncClient : public IClient {
public:
    void notifyCircuitBreaker(bool success) {
        if (circuitBreaker) {
            if (success) {
                circuitBreaker->OnSuccess();
            } else {
                circuitBreaker->OnFail();
            }
        }
    }

    void Call(RequestMessage *msg);

private:
    void CallWithRetry(RequestMessage *msg);

private:
    int retries;
    int timeout;
    TokenBucket *rateLimit;
    CircuitBreaker *circuitBreaker;
};

void AsyncClient::CallWithRetry(RequestMessage *msg) {
    bool success = false;

    for (int i = 0; retries + 1; i++) {
        http_client client(msg->GetURL());

        http_request request(methods::POST);
        for (auto head : msg->GetHTTPHeaders())
            request.headers().add(head.first, head.second);
        request.headers().add("Content-Type", "application/octet-stream");
        request.headers().add("Content-Length", std::to_string(msg->GetData().size()));

        auto reqStartTime = std::chrono::steady_clock::now();
        http_response resp = client.request(request).get();
        auto reqUsedTime = std::chrono::steady_clock::now() - reqStartTime;
        auto reqUsedTimeMS = std::chrono::duration_cast<std::chrono::microseconds>(reqUsedTime).count();

        resp.body().close();

        if (resp.status_code() != 200) {
            notifyCircuitBreaker(false);
            std::ostringstream oss;
            oss << "HTTP调用失败(%" << i << ") (" << reqUsedTimeMS << "ms) " << resp.status_code() << std::endl;
            LOG(WARNING) << oss.str() << std::endl;
            continue;
        }

        success = true;
        notifyCircuitBreaker(true);
        std::ostringstream oss;
        oss << "HTTP调用成功(%" << i << ") (" << reqUsedTimeMS << "ms) " << resp.status_code() << std::endl;
        LOG(INFO) << oss.str() << std::endl;
        break;
    }
}

void AsyncClient::Call(RequestMessage *msg) {
    if (circuitBreaker) {
        while (true) {

        }
    }

    rateLimit->GetToken(1);



}



