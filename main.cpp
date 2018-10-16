#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "acceptor/acceptor.hpp"
#include "kafka/producer.hpp"
#include "kafka/consumer.hpp"
#include "base/SignalHandler.hpp"
#include "config/config.hpp"

DEFINE_string(configFile, "./conf/bigpipe.json", "absolute path to bigpipe.json");

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    std::shared_ptr<Config> config = Config::ParseConfig(FLAGS_configFile);
    if (config == nullptr) {
        LOG(ERROR) <<  "配置文件加载失败: " << FLAGS_configFile << std::endl;
        return -1;
    }

    Acceptor acceptor;
    acceptor.Init(config);

    KafkaProducer producer;
    producer.Init(config);
    producer.SetJobChannel(acceptor.GetTaskChannel());

    KafkaConsumer consumer;
    consumer.Init(config);

    consumer.Start();
    producer.Start();
    acceptor.Start();

    SignalHandler::hookSIGINT();
    SignalHandler::waitForUserInterrupt();

    acceptor.Stop();
    producer.Stop();
    consumer.Stop();

    google::ShutdownGoogleLogging();
    gflags::ShutDownCommandLineFlags();

    return 0;
}