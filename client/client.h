

#pragma once

#include <tclDecls.h>
#include "../proto/reqmsg.hpp"

class IClient {
public:
    static IClient *CreateClient(ConsumerInfo *info) {
        CreateAsyncClient
    }
public:
    void Call(RequestMessage *message);
    int PendingCount();
};