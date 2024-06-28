//
// Created by 韩呈杰 on 2023/8/8.
//
#include "collect_info.h"
#include "common/Common.h"

namespace cloudMonitor {
    std::string CollectInfo::GetVersion() {
        return common::getVersion(false);
    }

    std::string CollectInfo::GetCompileTime() {
        return common::compileTime();
    }
}