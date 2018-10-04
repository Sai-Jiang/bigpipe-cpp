#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "config/config.h"

DEFINE_string(configFile, "./conf/bigpipe.json", "abs path to bigpipe.json");

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    auto config = Config::ParseConfig(FLAGS_configFile);
    if (config == nullptr) {
        LOG(ERROR) <<  "[热重启]解析配置失败" << std::endl;
        return -1;
    }

    LOG(INFO) << "[热重启]暂停handler" << std::endl;


    google::ShutdownGoogleLogging();
    gflags::ShutDownCommandLineFlags();

    return 0;
}