#pragma once

#include <vector>
#include <thread>
#include <random>
#include <cppkafka/cppkafka.h>
#include "../proto/reqmsg.hpp"
#include "../config/config.hpp"

class KafkaProducer {
public:
    KafkaProducer() = default;

public:
    bool Start();

private:
    int CalcPartition(int partitions, const std::string &partitionKey);
    void Pushing();

private:
    Config *bigConf;
    std::atomic<bool> IsRunning;
    cppkafka::Producer kafkaClient;
    std::shared_ptr<FixedSizeTaskChan> taskchan;
    std::vector<std::thread> producers;
    std::string kafkaServer;
    int retries;
};

int KafkaProducer::CalcPartition(int partitions, const std::string &partitionKey) {
    if (partitionKey.empty()) {
        std::random_device rd;
        return rd() % partitions;
    }
    std::hash<std::string> StrHash;
    return (int)StrHash(partitionKey) % partitions;
}

void KafkaProducer::Pushing() {
    while (IsRunning) {
        RequestMessage reqmsg;
        bool isok = taskchan->pop(reqmsg);    // may stall, sending one fake message each forwarder
        if (!isok) continue;                  // check again
        std::string topic = reqmsg.GetTopic();
        int partition = CalcPartition(bigConf->Kafka_topics[topic].Partitions, reqmsg.GetPartitionKey());
        std::string payload = reqmsg.ToJSON();
        kafkaClient.produce(cppkafka::MessageBuilder(topic).partition(partition).payload(payload));
        kafkaClient.flush();
    }
}

bool KafkaProducer::Start() {
//    cppkafka::Configuration conf;
//    conf.set("bootstrap.servers", kafkaServer);
//    conf.set("retries", retries);
//    cppkafka::Producer producer(conf);


    for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
        std::thread producer;

    }

}