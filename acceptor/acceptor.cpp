#include <unordered_map>
#include "../proto/reqbody.hpp"
#include "../proto/callresp.hpp"
#include "../proto/reqmsg.hpp"
#include "acceptor.hpp"

bool Acceptor::Init(const std::shared_ptr<Config> &pConf) {
    if (!bigConf) return false;

    bigConf = pConf;

    served::multiplexer mux;
    mux.handle("/rpc/call").get(std::bind(&Acceptor::RpcCallback, this, std::placeholders::_1, std::placeholders::_2));

    httpServer = std::make_unique<served::net::server>(new served::net::server(std::string("0.0.0.0"),
            std::to_string(bigConf->Http_server_port), mux));

    httpServer->set_read_timeout(bigConf->Http_server_read_timeout);
    httpServer->set_write_timeout(bigConf->Http_server_write_timeout);

    taskchan = std::make_shared<BoundedBlockingQueue<RequestMessage>>(bigConf->Http_server_handler_channel_size);

    return true;
}

bool Acceptor::Stop() {
    IsRunning = false;
    httpServer->stop();
    LOG(INFO) << "HTTP服务器关闭" << std::endl;
}

bool Acceptor::Start() {
    httpServer->run(1);
    LOG(INFO) << "HTTP服务器启动" << std::endl;
    IsRunning = true;
    return true;
}

void Acceptor::RpcCallback(served::response &resp, const served::request &req) {
    RequestBody reqbody;
    try {
        reqbody.ParseFromJSON(req.body());
    } catch (const std::exception &e) {
        CallResp status(-1, e.what(), std::string());
        resp << status.ToJSON();
    }
    RequestMessage reqmsg(req.headers(), reqbody);
    if (!taskchan->full()) {
        taskchan->put(reqmsg);  // Todo: double check, may block; especially when stop
        CallResp status(0, "发送成功", std::string());
        resp << status.ToJSON();
    } else {
        CallResp status(-1, "系统过载", std::string());
        resp << status.ToJSON();
    }
}

