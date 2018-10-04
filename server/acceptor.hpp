#pragma once

#include <memory>
#include <thread>
#include <string>
#include <served/served.hpp>
#include <glog/logging.h>

class Acceptor {
public:
    Acceptor(std::string addr, std::string port, int read_timeout, int write_timeout, served::multiplexer mux) :
            httpServer(new served::net::server(addr, port, mux)) {
        httpServer->set_read_timeout(read_timeout);
        httpServer->set_write_timeout(write_timeout);
    }
public:
    void Run() {
        httpServer->run(1);
        LOG(INFO) << "HTTP服务器启动" << std::endl;
    }
    void Stop() {
        httpServer->stop();
        LOG(INFO) << "HTTP服务器关闭" << std::endl;
    }
private:
    std::unique_ptr<served::net::server> httpServer;
};