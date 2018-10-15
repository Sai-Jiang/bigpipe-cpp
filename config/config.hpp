#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <exception>
#include <unordered_map>
#include <glog/logging.h>
#include <folly/json.h>
#include "../proto/acl.hpp"
#include "../kafka/consumer.hpp"

struct KafkaTopicAttr {
    int Partitions;
};

struct Config {
public:
    static std::unique_ptr<Config> ParseConfig(std::string path);
private:
    Config() = default;

public:
    std::string Log_directory;
    int Log_level;

    std::string Kafka_bootstrap_servers;
    std::unordered_map<std::string, KafkaTopicAttr> Kafka_topics;

    int Kafka_producer_retries;
    std::unordered_map<std::string, AccessControlList> Kafka_producer_acl;

    std::vector<ConsumerConf> Kafka_consumer_list;

    int Http_server_port;
    int Http_server_read_timeout;
    int Http_server_write_timeout;
    int Http_server_handler_channel_size;
};

std::unique_ptr<Config> Config::ParseConfig(std::string path) {
    std::ifstream ifs(path);
    std::string content{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};

    folly::dynamic dict = folly::parseJson(content);

    std::unique_ptr<Config> config(new Config);

    try {
        config->Log_directory = dict["log.directory"].asString();
        config->Log_level = int(dict["log.level"].asInt());

        config->Kafka_bootstrap_servers = dict["kafka.bootstrap.servers"].asString();
        config->Kafka_producer_retries = int(dict["kafka.producer.retries"].asInt());

        config->Http_server_port = int(dict["http.acceptor.port"].asInt());
        config->Http_server_read_timeout = int(dict["http.acceptor.read.timeout"].asInt());
        config->Http_server_write_timeout = int(dict["http.acceptor.write.timeout"].asInt());
        config->Http_server_handler_channel_size = int(dict["http.acceptor.handler.channel.size"].asInt());

        for (auto topicMap : dict["kafka.topics"]) {
            std::string name = topicMap["name"].asString();
            int partitions = int(topicMap["partitions"].asInt());
            config->Kafka_topics[name] = KafkaTopicAttr{.Partitions = partitions};
        }

        for (auto &aclMap : dict["kafka.producer.acl"]) {
            AccessControlList acl;
            acl.FromJSON(aclMap.asString());
            config->Kafka_producer_acl[acl.GetName()] = acl;
            if (config->Kafka_topics.count(acl.GetTopic()) == 0) {
                LOG(WARNING) << ("ACL中配置的topic: " + acl.GetTopic() + " 不存在,请检查kafka.topics.") << std::endl;
                return std::unique_ptr<Config>(nullptr);
            }
        }

        for (auto item : dict["kafka.consumer.list"]) {
            ConsumerConf consumerConf;
            consumerConf.Topic = item["topic"].asString();
            consumerConf.GroupId = item["groupId"].asString();
            consumerConf.RateLimit = int(item["rateLimit"].asInt());
            consumerConf.Retries = int(item["retries"].asInt());
            consumerConf.Timeout = int(item["timeout"].asInt());
            consumerConf.Concurrency = int(item["concurrency"].asInt());

            if (item.count("circuitBreaker") > 0) {
                auto circuitMap = item["circuitBreaker"];
                consumerConf.CircuiteBreakerConf->RecoverPeriod = int(circuitMap["recoverPeriod"].asDouble());
                consumerConf.CircuiteBreakerConf->BreakPeriod = int(circuitMap["breakPeriod"].asDouble());
                consumerConf.CircuiteBreakerConf->WinSize = int(circuitMap["winSize"].asDouble());
                consumerConf.CircuiteBreakerConf->HealthRate = int(circuitMap["healthRate"].asDouble());
                consumerConf.CircuiteBreakerConf->MinStats = int(circuitMap["minStats"].asDouble());
                if (consumerConf.CircuiteBreakerConf->WinSize <= 0 ||
                        consumerConf.CircuiteBreakerConf->MinStats <= 0 ||
                        consumerConf.CircuiteBreakerConf->HealthRate <= 0 ||
                        consumerConf.CircuiteBreakerConf->HealthRate > 100) {
                    LOG(WARNING) << "consumer配置的熔断器参数有误, 请检查一下." << std::endl;
                    return std::unique_ptr<Config>(nullptr);
                }
            }
            config->Kafka_consumer_list.push_back(consumerConf);
            if (config->Kafka_topics.count(consumerConf.Topic) == 0) {
                LOG(WARNING) << ("consumer中配置的topic: ") << consumerConf.Topic << " 不存在,请检查kafka.topics." << std::endl;
                return std::unique_ptr<Config>(nullptr);
            }
        }
    } catch (std::exception & e) {
        LOG(ERROR) << e.what() << std::endl;
        return std::unique_ptr<Config>(nullptr);
    }

    return config;
}
