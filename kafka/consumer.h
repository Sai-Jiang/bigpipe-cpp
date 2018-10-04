#pragma once

#include <cppkafka/consumer.h>
#include "../client/client.h"

class KafkaConsumerWrapper {
private:
    std::vector<cppkafka::Consumer> clients;

};