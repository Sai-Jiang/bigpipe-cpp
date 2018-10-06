#include "server.hpp"
#include <unordered_map>
#include "../proto/reqbody.hpp"
#include "../proto/callresp.hpp"
#include "../proto/reqmsg.hpp"

bool Server::Init(Config *bigConf) {
    if (!bigConf) return false;

    served::multiplexer mux;
    mux.handle("/rpc/call").get(std::bind(&Server::RpcCallback, this, std::placeholders::_1, std::placeholders::_2));

    acceptor = std::make_unique<Acceptor>(std::string("0.0.0.0"), std::to_string(bigConf->Http_server_port),
                                          bigConf->Http_server_read_timeout, bigConf->Http_server_write_timeout, mux);

    taskchan = std::make_unique<FixedSizeChannel>(bigConf->Http_server_handler_channel_size);

    kafkaProducer = std::make_unique<KafkaProducerWrapper>();
    kafkaProducer->SetBootstrapServer(bigConf->Kafka_bootstrap_servers);
    kafkaProducer->SetRetries(bigConf->Kafka_producer_retries);

    return true;
}

bool Server::Stop() {
    IsRunning = false;

    acceptor->Stop();
    for (auto &thread : forwarders)
        thread.join();
}

bool Server::Start(int threadnum) {
    for (int i = 0; i < threadnum; i++)
        forwarders.push_back(std::thread(ForwardingTask));

    acceptor->Run();

    IsRunning = true;

    return true;
}

void Server::ForwardingTask() {
    auto tid = std::this_thread::get_id();

    while (IsRunning) {
        RequestMessage reqmsg = taskchan->pop();    // may stall, sending one fake message each forwarder
        if (!IsRunning) break;                      // check again
        auto topic = reqmsg.GetTopic();
        auto partition = CalcPartition(bigConf->Kafka_topics[topic].Partitions, reqmsg.GetPartitionKey());
        kafkaProducer->SendMessage(reqmsg.GetTopic(), partition, reqmsg);
    }
}

void Server::RpcCallback(served::response &resp, const served::request &req) {
    RequestBody reqbody;
    try {
        reqbody.ParseFromJSON(req.body());
    } catch (const std::exception &e) {
        CallResp status(-1, e.what(), std::string());
        resp << status.ToJSON();
    }

    RequestMessage reqmsg(req.headers(), reqbody);
    if (taskchan->bounded_push(reqmsg)) {
        CallResp status(0, "发送成功", std::string());
        resp << status.ToJSON();
    } else {
        CallResp status(-1, "系统过载", std::string());
        resp << status.ToJSON();
    }
}

int Server::CalcPartition(int partitions, const std::string &partitionKey) {
    if (partitionKey.empty()) {
        std::random_device rd;
        return rd() % partitions;
    }
    std::hash<std::string> StrHash;
    return (int)StrHash(partitionKey) % partitions;
}