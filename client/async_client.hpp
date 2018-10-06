#pragma once

#include <cpprest/http_client.h>
#include "../proto/reqmsg.hpp"

using namespace web::http::client;

class AsyncClient {
public:

private:
    void CallWithRetry(RequestMessage *msg);

private:

};

void AsyncClient::CallWithRetry(RequestMessage *msg) {

}

