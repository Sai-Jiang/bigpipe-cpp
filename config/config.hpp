#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <exception>
#include <unordered_map>
#include <glog/logging.h>
#include <folly/json.h>
#include "../proto/acl.hpp"

struct TopicInfo {
    int Partitions;
};

struct CircuitBreakerInfo {
    int BreakPeriod;
    int RecoverPeriod;
    int WinSize;
    int MinStats;
    double HealthRate;
};

struct ConsumerInfo {
    std::string Topic;
    std::string GroupId;
    int RateLimit;
    int Retries;
    int Timeout;
    int Concurrency;
    CircuitBreakerInfo *CircuiteBreakerInfo;
};

struct Config {
public:
    static Config *ParseConfig(std::string path);
private:
    Config() = default;

public:
    std::string Log_directory;
    int Log_level;

    std::string Kafka_bootstrap_servers;
    std::unordered_map<std::string, TopicInfo> Kafka_topics;

    int Kafka_producer_retries;
    std::unordered_map<std::string, > Kafka_producer_acl;

    std::vector<ConsumerInfo> Kafka_consumer_list;

    int Http_server_port;
    int Http_server_read_timeout;
    int Http_server_write_timeout;
    int Http_server_handler_channel_size;
};

Config *Config::ParseConfig(std::string path) {
    std::ifstream ifs(path);
    std::string content{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};

    folly::dynamic dict = folly::parseJson(content);

    Config *config = new Config;

    try {
        config->Log_directory = dict["log.directory"].asString();
        config->Log_level = int(dict["log.level"].asInt());

        config->Kafka_bootstrap_servers = dict["kafka.bootstrap.servers"].asString();
        config->Kafka_producer_retries = int(dict["kafka.producer.retries"].asInt());

        config->Http_server_port = int(dict["http.server.port"].asInt());
        config->Http_server_read_timeout = int(dict["http.server.read.timeout"].asInt());
        config->Http_server_write_timeout = int(dict["http.server.write.timeout"].asInt());
        config->Http_server_handler_channel_size = int(dict["http.server.handler.channel.size"].asInt());

        for (auto topicMap : dict["kafka.topics"]) {
            std::string name = topicMap["name"].asString();
            int partitions = int(topicMap["partitions"].asInt());
            config->Kafka_topics[name] = TopicInfo{Partitions: partitions};
        }

        for (auto aclMap : dict["kafka.producer.acl"]) {
            std::string name = aclMap["name"].asString();
            std::string secret = aclMap["secret"].asString();
            std::string topic = aclMap["topic"].asString();
            config->Kafka_producer_acl[name] = A{Name: name, Secret: secret, Topic: topic};
            if (config->Kafka_topics.count(topic) == 0) {
                LOG(WARNING) << ("ACL中配置的topic: " + topic + " 不存在,请检查kafka.topics.") << std::endl;
                delete(config);
                return nullptr;
            }
        }

        for (auto item : dict["kafka.consumer.list"]) {
            ConsumerInfo consumerInfo;
            consumerInfo.Topic = item["topic"].asString();
            consumerInfo.GroupId = item["groupId"].asString();
            consumerInfo.RateLimit = int(item["rateLimit"].asInt());
            consumerInfo.Retries = int(item["retries"].asInt());
            consumerInfo.Timeout = int(item["timeout"].asInt());
            consumerInfo.Concurrency = int(item["concurrency"].asInt());
            if (item.count("circuitBreaker") > 0) {
                auto circuitMap = item["circuitBreaker"];
                consumerInfo.CircuiteBreakerInfo->RecoverPeriod = int(circuitMap["recoverPeriod"].asDouble());
                consumerInfo.CircuiteBreakerInfo->BreakPeriod = int(circuitMap["breakPeriod"].asDouble());
                consumerInfo.CircuiteBreakerInfo->WinSize = int(circuitMap["winSize"].asDouble());
                consumerInfo.CircuiteBreakerInfo->HealthRate = int(circuitMap["healthRate"].asDouble());
                consumerInfo.CircuiteBreakerInfo->MinStats = int(circuitMap["minStats"].asDouble());
                if (consumerInfo.CircuiteBreakerInfo->WinSize <= 0 ||
                    consumerInfo.CircuiteBreakerInfo->MinStats <= 0 ||
                    consumerInfo.CircuiteBreakerInfo->HealthRate <= 0 ||
                    consumerInfo.CircuiteBreakerInfo->HealthRate > 100) {
                    LOG(WARNING) << "consumer配置的熔断器参数有误, 请检查一下." << std::endl;
                    delete(config);
                    return nullptr;
                }
            }
            config->Kafka_consumer_list.push_back(consumerInfo);
            if (config->Kafka_topics.count(consumerInfo.Topic) == 0) {
                LOG(WARNING) << ("consumer中配置的topic: ") << consumerInfo.Topic << " 不存在,请检查kafka.topics." << std::endl;
                delete(config);
                return nullptr;
            }
        }
    } catch (std::exception & e) {
        LOG(ERROR) << e.what() << std::endl;
        delete(config);
        config = nullptr;
    }

    return config;
}
