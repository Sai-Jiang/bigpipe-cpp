#pragma once

#include <tclDecls.h>
#include "../proto/reqmsg.hpp"

class IClient {
public:
//    static IClient *CreateClient()

public:
    virtual void Call(RequestMessage &msg) = 0;
    virtual int PendingCount() = 0;
};