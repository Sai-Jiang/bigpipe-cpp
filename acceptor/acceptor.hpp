#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <served/served.hpp>
#include <boost/lockfree/queue.hpp>
#include "../proto/reqmsg.hpp"
#include "../config/config.hpp"
#include "../kafka/producer.hpp"
#include "../config/config.hpp"

class Acceptor {
public:
    Acceptor() = default;

public:
    bool Init(Config *bigConf);
    bool Start();
    bool Stop();

public:
    std::shared_ptr<FixedSizeTaskChan> GetTaskChannel() { return taskchan; }

private:
    void RpcCallback(served::response &resp, const served::request &req);

private:
    std::atomic<bool> IsRunning;
    std::weak_ptr<Config> bigConf;
    std::unique_ptr<served::net::server> httpServer;
    std::shared_ptr<FixedSizeTaskChan> taskchan;
};


