#pragma once

#include <string>
#include <folly/dynamic.h>
#include <folly/json.h>

class CallResp {
public:
    CallResp(int errcode, std::string msg, folly::dynamic data) : errcode(errcode), msg(msg), data(data) {}
public:
    std::string ToJSON() {
        folly::dynamic status = folly::dynamic::object;
        status["errno"] = errcode;
        status["msg"] = msg;
        status["data"] = data;
        return folly::toJson(status);
    }
private:
    int errcode;
    std::string msg;
    folly::dynamic data;
};