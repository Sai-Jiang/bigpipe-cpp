#pragma once

#include <memory>
#include <string>
#include <served/served.hpp>
#include <boost/lockfree/queue.hpp>
#include "../proto/reqmsg.hpp"
#include "../config/config.h"
#include "acceptor.hpp"

class Server {
public:
    Server() = default;

public:
    bool Init(Config *bigConf);
    bool Start(int threadnum);
    bool Stop();

private:
    void RpcCallback(served::response &resp, const served::request &req);

private:
    std::unique_ptr<Acceptor> acceptor;
    boost::lockfree::queue<RequestMessage> taskchan;
    std::vector<std::thread> threads;
};

bool Server::Init(Config *bigConf) {
    if (!bigConf) return false;

    served::multiplexer mux;
    mux.handle("/rpc/call").get(std::bind(&Server::RpcCallback, this, std::placeholders::_1, std::placeholders::_2));

    acceptor = std::make_unique<Acceptor>(std::string("0.0.0.0"), std::to_string(bigConf->Http_server_port),
            bigConf->Http_server_read_timeout, bigConf->Http_server_write_timeout, mux);

    return true;
}

bool Server::Stop() {
    acceptor->Stop();
}

bool Server::Start(int threadnum) {

    acceptor->Run();
}

