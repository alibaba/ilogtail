#pragma once

#include <string>
#include <vector>

#include "config/NewConfig.h"

namespace logtail {
struct ConfigDiff {
    bool IsEmpty() { return mRemoved.empty() && mAdded.empty() && mModified.empty(); }

    std::vector<NewConfig> mAdded;
    std::vector<NewConfig> mModified;
    std::vector<std::string> mRemoved;
    std::vector<std::string> mUnchanged; // 过渡使用，仅供插件系统用
};
} // namespace logtail