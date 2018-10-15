#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <glog/logging.h>
#include <cppkafka/consumer.h>
#include "../client/circuit_breaker.hpp"
#include "../config/config.hpp"

struct ConsumerConf {
    std::string Topic;
    std::string GroupId;
    int RateLimit;
    int Retries;
    int Timeout;
    int Concurrency;
    std::unique_ptr<CircuitBreakerConf> CircuitBreakerConf;
};

class KafkaConsumer {
public:
    KafkaConsumer() = default;

public:
    bool Init(const std::shared_ptr<Config> &pConf);
    bool Start();
    bool Stop();

private:
    void ConsumeLoop(const cppkafka::Queue &queue);

private:
    std::atomic<bool> IsRunning;
    std::shared_ptr<Config> bigConf;
    std::vector<cppkafka::Consumer> clients;
    std::vector<cppkafka::Queue> queues;
    std::vector<std::thread> threads;
};

bool KafkaConsumer::Init(const std::shared_ptr<Config> &pConf) {
    bigConf = pConf;

    for (auto &consumerConf : bigConf->Kafka_consumer_list) {
        cppkafka::Configuration kafkaConf;

        kafkaConf.set("bootstrap.servers", bigConf->Kafka_bootstrap_servers);
        kafkaConf.set("group.id", consumerConf.GroupId);
        kafkaConf.set("heartbeat.interval.ms", 1000);
        kafkaConf.set("session.timeout.ms", 30000);
        kafkaConf.set("auto.offset.reset", "latest");
        kafkaConf.set("enable.auto.commit", true);
        kafkaConf.set("auto.commit.interval.ms", 1000);

        cppkafka::Consumer cli(kafkaConf);
        cli.subscribe(std::vector<std::string>{consumerConf.Topic});
        cli.set_assignment_callback([&](cppkafka::TopicPartitionList& topicPartitionList) {
            LOG()
            cli.assign(topicPartitionList);
        });
        cli.set_revocation_callback([&](const cppkafka::TopicPartitionList& topicPartitionList) {
            cli.unassign();
        });
        cli.set_rebalance_error_callback([&](cppkafka::Error error) {
            LOG(FATAL) << "%% Error: " << error.to_string() << std::endl;
        });

        queues.push_back(std::move(cli.get_consumer_queue()));
        clients.push_back(std::move(cli));
    }

    return true;
}

void KafkaConsumer::ConsumeLoop(const cppkafka::Queue &queue) {
    while (IsRunning) {
        auto msg = queue.consume();

    }
}

bool KafkaConsumer::Start() {
    IsRunning = true;

}

bool KafkaConsumer::Stop() {
    IsRunning = false;

}