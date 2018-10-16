#pragma once

#include <vector>
#include <thread>
#include <random>
#include <glog/logging.h>
#include <cppkafka/cppkafka.h>
#include "../proto/reqmsg.hpp"
#include "../config/config.hpp"
#include "../base/BoundedBlockingQueue.hpp"


class KafkaProducer {
public:
    KafkaProducer() = default;

public:
    bool Init(const std::shared_ptr<Config>& bigConf);
    bool Start();
    bool Stop();
    void SetJobChannel(const std::shared_ptr<BoundedBlockingQueue<RequestMessage>>& taskchan);

private:
    int CalcPartition(int partitions, const std::string &partitionKey);
    void Pushing();
    void EventsHandler(cppkafka::Producer& producer, const cppkafka::Message&);

private:
    std::shared_ptr<Config> bigConf;
    std::atomic<bool> IsRunning;
    std::unique_ptr<cppkafka::Producer> kafkaClient;
    std::shared_ptr<BoundedBlockingQueue<RequestMessage>> taskchan;
    std::vector<std::thread> producers;
    static const int NPRODUCERS = 1;
};

void KafkaProducer::EventsHandler(cppkafka::Producer& producer, const cppkafka::Message& msg) {
    if (msg.get_error()) {
        LOG(ERROR) << "投递失败: " << msg.get_error().to_string() << std::endl;
    } else {
        LOG(INFO) << "投递成功 Topic " << msg.get_topic() << " " << msg.get_partition() << " "
        << "at offset " << msg.get_offset() << std::endl;
    }
}

bool KafkaProducer::Init(const std::shared_ptr<Config>& pConf) {
    bigConf = pConf;

    cppkafka::Configuration conf;
    conf.set("bootstrap.servers", bigConf->Kafka_bootstrap_servers);
    conf.set("retries", bigConf->Kafka_producer_retries);
    conf.set_delivery_report_callback(std::bind(&KafkaProducer::EventsHandler,
            this, std::placeholders::_1, std::placeholders::_2));

    kafkaClient = std::make_unique<cppkafka::Producer>(conf);
}

void KafkaProducer::SetJobChannel(const std::shared_ptr<BoundedBlockingQueue<RequestMessage>>& chan) {
    taskchan = chan;
}

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
        reqmsg = taskchan->take();  // Fixme:  可能存在阻塞的问题，尤其是执行退出逻辑时；
        std::string topic = reqmsg.GetTopic();
        if (topic == "") continue;
        int partition = CalcPartition(bigConf->Kafka_topics[topic].Partitions, reqmsg.GetPartitionKey());
        std::string payload = reqmsg.ToJSON();
        kafkaClient->produce(cppkafka::MessageBuilder(topic).partition(partition).payload(payload)); // thread-safe
    }
}

bool KafkaProducer::Start() {
    if (!taskchan.use_count()) {
        LOG(ERROR) << "尚未设置Producer的JobChannel" << std::endl;
        return false;
    }
    for (int i = 0; i < NPRODUCERS; i++)
        producers.emplace_back(std::thread(std::bind(&KafkaProducer::Pushing, this)));
    IsRunning = true;
    LOG(INFO) << "KafkaProducer启动成功" << std::endl;
    return true;
}

bool KafkaProducer::Stop() {
    IsRunning = false;
    for (int i = 0; i < NPRODUCERS; i++) {
        taskchan->put(RequestMessage());    // Empty RequestMessage() for exit notification
        producers[i].join();
    }
    while (kafkaClient->get_out_queue_length() > 0) {
        LOG(INFO) << "Stopping KafkaProducer: flushing Kafka" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG(INFO) << "KafkaProducer关闭成功" << std::endl;
    return true;
}