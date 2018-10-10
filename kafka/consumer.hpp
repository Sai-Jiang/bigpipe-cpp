#pragma once

#include <string>
#include <vector>
#include <cppkafka/consumer.h>
#include "../config/config.hpp"
#include "../client/circuit_breaker.hpp"

struct ConsumerConf {
    std::string Topic;
    std::string GroupId;
    int RateLimit;
    int Retries;
    int Timeout;
    int Concurrency;
    CircuitBreakerConf *CircuiteBreakerConf;
};

class Consumer {
public:
    Consumer(Config *conf);
private:
    std::vector<cppkafka::Consumer> clients;


};

Consumer::Consumer(Config *conf) {
    for (auto consumerConf : conf->Kafka_consumer_list) {
        cppkafka::Configuration kafkaConf;

        kafkaConf.set("bootstrap.servers", conf->Kafka_bootstrap_servers);
        kafkaConf.set("group.id", consumerConf.GroupId);
        kafkaConf.set("heartbeat.interval.ms", 1000);
        kafkaConf.set("session.timeout.ms", 30000);
        kafkaConf.set("auto.offset.reset", "latest");
        kafkaConf.set("enable.auto.commit", true);
        kafkaConf.set("auto.commit.interval.ms", 1000);

        cppkafka::Consumer cli(kafkaConf);
        cli.subscribe(std::vector<std::string>{consumerConf.Topic});
        clients.push_back(cli);

    }

}