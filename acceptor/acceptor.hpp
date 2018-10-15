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
#include "../base/BoundedBlockingQueue.hpp"

class Acceptor {
public:
    Acceptor() = default;

public:
    bool Init(const std::shared_ptr<Config> &bigConf);
    bool Start();
    bool Stop();

public:
    std::shared_ptr<BoundedBlockingQueue<RequestMessage>> GetTaskChannel() { return taskchan; }

private:
    void RpcCallback(served::response &resp, const served::request &req);

private:
    std::atomic<bool> IsRunning;
    std::shared_ptr<Config> bigConf;
    std::unique_ptr<served::net::server> httpServer;
    std::shared_ptr<BoundedBlockingQueue<RequestMessage>> taskchan;
};