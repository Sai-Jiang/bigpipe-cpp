#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <served/served.hpp>
#include <boost/lockfree/queue.hpp>
#include "acceptor.hpp"
#include "../proto/reqmsg.hpp"
#include "../config/config.hpp"
#include "../kafka/producer.hpp"
#include "../config/config.hpp"

class Server {
public:
    Server() = default;

public:
    bool Init(Config *bigConf);
    bool Start(int threadnum);
    bool Stop();

private:
    void RpcCallback(served::response &resp, const served::request &req);
    void ForwardingTask();
    int CalcPartition(int partitions, const std::string &partitionKey);

private:
    typedef boost::lockfree::queue<RequestMessage, boost::lockfree::fixed_sized<true>> FixedSizeChannel;

    std::atomic<bool> IsRunning;

    std::unique_ptr<Config> bigConf;
    std::unique_ptr<Acceptor> acceptor;
    std::unique_ptr<FixedSizeChannel> taskchan;
    std::vector<std::thread> forwarders;

    std::unique_ptr<KafkaProducerWrapper> kafkaProducer;
};