#pragma once

#include <string>
#include <fstream>
#include <unordered_map>
#include <glog/logging.h>
#include <folly/json.h>
#include <folly/dynamic.h>
#include <algorithm>
#include "acl.hpp"

class RequestBody {
public:
//    RequestBody() = default;

public:
    bool ParseFromJSON(std::string json);
    std::string ToJSON();

public:
    void SetACL(AccessControlList acl) { acl = acl; }
    void SetURL(std::string url) { url = url; }
    void SetPartitionKey(std::string partitionKey) { partitionKey = partitionKey; }
    void SetData(std::string data) { data = data; }

public:
    AccessControlList GetACL() { return acl; }
    std::string GetURL() { return url; }
    std::string GetPartitionKey() { return partitionKey; }
    std::string GetData() { return data; }

private:
    AccessControlList acl;
    std::string partitionKey;
    std::string url;
    std::string data;
};

bool RequestBody::ParseFromJSON(std::string body) {
    std::ifstream ifs(body);
    std::string content{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    folly::dynamic dict = folly::parseJson(content);

    url = dict["url"].asString();
    data = dict["data"].asString();
    partitionKey = dict["partitionKey"].asString();
    acl.FromJSON(dict["acl"].asString());

    return true;
}

std::string RequestBody::ToJSON() {
    folly::dynamic obj = folly::dynamic::object;

    obj["url"] = url;
    obj["data"] = data;
    obj["partitionKey"] = partitionKey;
    obj["acl"] = acl.ToJSON();

    return folly::toJson(obj);
}