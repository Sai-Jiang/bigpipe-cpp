#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <folly/json.h>
#include <folly/dynamic.h>
#include <boost/lockfree/queue.hpp>
#include "reqbody.hpp"


class RequestMessage {
public:
    RequestMessage() = default;
    RequestMessage(std::unordered_map<std::string, std::string> headers, RequestBody reqbody) : headers(headers) {
        topic = reqbody.GetACL().GetTopic();
        partitionKey = reqbody.GetPartitionKey();
        url = reqbody.GetURL();
        data = reqbody.GetData();
    }

public:
    bool FromJSON(std::string json);
    std::string ToJSON();

public:
    void SetURL(std::string url) { url = url; }
    void SetData(std::string data) { data = data; }
    void SetHTTPHeaders(std::map<std::string, std::string> headers) { headers = headers; }

public:
    std::unordered_map<std::string, std::string> GetHTTPHeaders() { return headers; }
    std::string GetURL() { return url; }
    std::string GetData() { return data; }
    std::string GetTopic() { return topic; }
    std::string GetPartitionKey() { return partitionKey; }

private:
    std::unordered_map<std::string, std::string> headers;
    std::string partitionKey;
    std::string topic;
    std::string url;
    std::string data;
};

bool RequestMessage::FromJSON(std::string json) {
    std::ifstream ifs(json);
    std::string content{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    folly::dynamic dict = folly::parseJson(content);

    url = dict["url"].asString();
    data = dict["data"].asString();
    for (auto header : dict["headers"].items())
        headers[header.first.asString()] = header.second.asString();

    return true;
}

std::string RequestMessage::ToJSON() {
    folly::dynamic obj = folly::dynamic::object;

    obj["headers"] = folly::dynamic::object;
    for (auto &header : headers)
        obj["headers"][header.first] = header.second;

    obj["url"] = url;
    obj["data"] = data;
    obj["topic"] = topic;

    return folly::toJson(obj);
}