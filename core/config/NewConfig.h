#pragma once

#include <string>
#include <memory>

#include "table/Table.h"

namespace logtail {
struct NewConfig {
    std::string mName;
    std::unique_ptr<Table> mDetail;
};

bool operator==(const NewConfig& lhs, const NewConfig &rhs);
bool operator!=(const NewConfig& lhs, const NewConfig &rhs);
} // namespace logtail
