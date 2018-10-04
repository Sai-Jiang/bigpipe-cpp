#include "server.hpp"
#include <unordered_map>
#include "../proto/reqbody.hpp"
#include "../proto/callstatus.hpp"
#include "../proto/reqmsg.hpp"

void Server::RpcCallback(served::response &resp, const served::request &req) {
    RequestBody reqbody;

    try {
        reqbody.ParseFromJSON(req.body());
    } catch (const std::exception &e) {
        CallStatus status(-1, e.what(), std::string());
        resp << status.ToJSON();
    }

    RequestMessage reqmsg(std::move(req.headers()), reqbody);


}

