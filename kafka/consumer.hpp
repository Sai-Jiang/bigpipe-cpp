#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <glog/logging.h>
#include <cppkafka/consumer.h>
#include "../client/circuit_breaker.hpp"
#include "../proto/reqmsg.hpp"
#include "../config/config.hpp"
#include "../client/async_client.hpp"

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
    std::shared_ptr<Config> bigConf;
    std::atomic<bool> IsRunning;
    std::unique_ptr<cppkafka::Consumer> client;
    std::vector<std::thread> threads;
    std::unique_ptr<AsyncClient> asyncClient;
    static const int NConsumers = 1;
};

bool KafkaConsumer::Init(const std::shared_ptr<Config> &pConf) {
    bigConf = pConf;

    cppkafka::Configuration kafkaConf;
    kafkaConf.set("bootstrap.servers", bigConf->Kafka_bootstrap_servers);
    kafkaConf.set("group.id", bigConf->Kafka_consumer.GroupId);
    kafkaConf.set("heartbeat.interval.ms", 1000);
    kafkaConf.set("session.timeout.ms", 30000);
    kafkaConf.set("auto.offset.reset", "latest");
    kafkaConf.set("enable.auto.commit", true);
    kafkaConf.set("auto.commit.interval.ms", 1000);

    client = std::make_unique<cppkafka::Consumer>(kafkaConf);
    client->subscribe(std::vector<std::string>{bigConf->Kafka_consumer.Topic});
    client->set_assignment_callback([&](cppkafka::TopicPartitionList& topicPartitionList) {
        client->assign(topicPartitionList);
    });
    client->set_revocation_callback([&](const cppkafka::TopicPartitionList& topicPartitionList) {
        client->unassign();
    });
    client->set_rebalance_error_callback([&](cppkafka::Error error) {
        LOG(FATAL) << "%% Error: " << error.to_string() << std::endl;
    });

    asyncClient = std::make_unique<AsyncClient>(bigConf->Kafka_consumer.Concurrency,
                                                bigConf->Kafka_consumer.Retries,
                                                bigConf->Kafka_consumer.Timeout);
    return true;
}

void KafkaConsumer::ConsumeLoop(const cppkafka::Queue &queue) {
    while (IsRunning) {
        cppkafka::Message kafkaMsg = queue.consume();
        if (kafkaMsg.is_eof()) break;
        RequestMessage reqmsg;
        if (reqmsg.FromJSON(kafkaMsg.get_payload())) { // Fixme: catch exception
            asyncClient->AsyncCall(reqmsg);
        } else {
            LOG(ERROR) << "消息格式错误: " << kafkaMsg.get_payload() << std::endl;
        }
    }
    LOG(INFO) << "KafkaConsumer::ConsumeLoop exit" << std::endl;
}

bool KafkaConsumer::Start() {
    IsRunning = true;
    asyncClient->Start();
    for (int i = 0; i < NConsumers; i++) {
        threads.emplace_back(std::thread(std::bind(&KafkaConsumer::ConsumeLoop,
                this, client->get_consumer_queue())));
    }
    LOG(INFO) << "KafkaProducer启动成功" << std::endl;
    return true;
}

bool KafkaConsumer::Stop() {
    IsRunning = false;
    for (int i = 0; i < NConsumers; i++)
        threads[i].join();
    asyncClient->Stop();
    LOG(INFO) << "KafkaProducer关闭成功" << std::endl;
    return true;
}