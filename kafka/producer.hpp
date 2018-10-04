#pragma once

#include <string>
#include <functional>
#include <random>
#include <cppkafka/cppkafka.h>
#include "../proto/reqmsg.hpp"
#include "../config/config.h"

class KafkaProducerWrapper {
public:
    KafkaProducerWrapper() : conf(new cppkafka::Configuration) {}
    ~KafkaProducerWrapper();

public:
    void SetBootstrapServer(std::string server) { conf->set("bootstrap.servers", server); };
    void SetRetries(int retries) { conf->set("retries", retries); };

public:
    bool Start();

public:
    void SendMessage(std::string topic, int partition, RequestMessage &message);

//private:
//    int getPartition(int partitions, const std::string &partitionKey);

private:
    std::unique_ptr<cppkafka::Producer> client;
    std::unique_ptr<cppkafka::Configuration> conf;
};

bool KafkaProducerWrapper::Start() {
    client.reset(new cppkafka::Producer(*conf));
    return true;
}

void KafkaProducerWrapper::SendMessage(std::string topic, int partition, RequestMessage &message) {
    auto payload = message.ToJSON();
    client->produce(cppkafka::MessageBuilder(topic).partition(partition).payload(payload));
    client->flush();
}

//int KafkaProducerWrapper::getPartition(int partitions, const std::string &partitionKey) {
//    if (partitionKey.empty()) {
//        std::random_device rd;
//        return rd() % partitions;
//    }
//    std::hash<std::string> StrHash;
//    return (int)StrHash(partitionKey) % partitions;
//}