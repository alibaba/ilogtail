#include "table/YAMLTable.h"

namespace logtail {
bool YAMLTable::parse(const std::string& content) {
    return false;
}

std::string YAMLTable::serialize() const {
    return std::string();
}

bool YAMLTable::is_null() const {
    return false;
}

bool operator==(const YAMLTable& lhs, const YAMLTable& rhs) {
    // TODO: yaml的==不是深比较，需要重写
    return lhs.mRoot == rhs.mRoot;
}

bool operator!=(const YAMLTable& lhs, const YAMLTable& rhs) {
    return !(lhs == rhs);
}
} // namespace logtail
