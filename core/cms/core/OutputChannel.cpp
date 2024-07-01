//
// Created by 韩呈杰 on 2023/5/4.
//
#include "OutputChannel.h"
#include "common/Payload.h"

namespace argus {
    std::string OutputChannel::ToPayloadString(double value) {
        return ::ToPayloadString(value);
    }
}
