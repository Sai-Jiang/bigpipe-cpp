#pragma once

#include <string>
#include <folly/json.h>
#include <folly/dynamic.h>

class AccessControlList {
public:
    AccessControlList() = default;

public:
    std::string GetTopic() { return topic; }
    std::string GetName() { return name; }
    std::string GetSecret() { return secret; }

public:
    void SetTopic(std::string topic) { topic = topic; }
    void SetName(std::string name) { name = name; }
    void SetSecret(std::string secret) { secret = secret; }

public:
    bool FromJSON(std::string json);
    std::string ToJSON();

private:
    std::string topic;  // optional in RequestBody
    std::string name;
    std::string secret;
};

bool AccessControlList::FromJSON(std::string json) {
    folly::dynamic dict = folly::parseJson(json);

    try {
        name = dict["name"].asString();
        secret = dict["secret"].asString();
        if (dict.count("topic") > 0)
            topic = dict["topic"].asString();
    } catch (std::exception &e) {
        LOG(ERROR) << e.what() << std::endl;
        return false;
    }

    return true;
}

std::string AccessControlList::ToJSON() {
    folly::dynamic obj = folly::dynamic::object;

    obj["name"] = name;
    obj["secret"] = secret;
    obj["topic"] = topic;

    return folly::toJson(obj);
}