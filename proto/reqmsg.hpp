#pragma once

#include <map>
#include <string>
#include <vector>
#include <folly/json.h>
#include <folly/dynamic.h>
#include "reqbody.hpp"

struct RequestMessage {
public:
    RequestMessage() = default;
    RequestMessage(std::map<std::string, std::string> headers, RequestBody reqbody) : headers(headers) {
        url = reqbody.GetURL();
        data = reqbody.GetData();
    }

public:
    bool FromJSON(std::string json);
    std::string ToJSON();

public:
    void SetURL(std::string url) { url = url; }
    void SetPartition(int partition) { partition = partition; }
    void SetData(std::string data) { data = data; }
    void SetHTTPHeaders(std::map<std::string, std::string> headers) { headers = headers; }

public:
    std::map<std::string, std::string> GetHTTPHeaders() { return headers; }
    std::string GetURL() { return url; }
    std::string GetData(std::string data) { return data; }

private:
    std::map<std::string, std::string> headers;
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

    return folly::toJson(obj);
}