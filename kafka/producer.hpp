#pragma once

#include <string>
#include <thread>
#include <functional>
#include <random>
#include <cppkafka/cppkafka.h>
#include "../proto/reqmsg.hpp"
#include "../config/config.hpp"

class KafkaProducerWrapper {
public:
    KafkaProducerWrapper() : conf(new cppkafka::Configuration), IsRunning(false) {}
    ~KafkaProducerWrapper() = default;

public:
    void SetBootstrapServer(std::string server) { conf->set("bootstrap.servers", server); };
    void SetRetries(int retries) { conf->set("retries", retries); };

public:
    bool Start();
    bool Stop();

public:
    void SendMessage(std::string topic, int partition, RequestMessage &message);

private:
    std::unique_ptr<cppkafka::Configuration> conf;
    std::unique_ptr<cppkafka::Producer> client;
    std::atomic<bool> IsRunning;
};

bool KafkaProducerWrapper::Start() {
    client = std::make_unique<cppkafka::Producer>(*conf);
    IsRunning = true;
    return true;
}

bool KafkaProducerWrapper::Stop() {
    while (client->get_out_queue_length() > 0)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    IsRunning = false;
    return true;
}

void KafkaProducerWrapper::SendMessage(std::string topic, int partition, RequestMessage &message) {
    auto payload = message.ToJSON();
    client->produce(cppkafka::MessageBuilder(topic).partition(partition).payload(payload));
    client->flush();
}