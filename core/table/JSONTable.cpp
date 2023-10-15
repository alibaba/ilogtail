#include "table/JSONTable.h"

namespace logtail {
bool JSONTable::parse(const std::string& content) {
    return false;
}

std::string JSONTable::serialize() const {
    return std::string();
}

bool JSONTable::is_null() const {
    return false;
}

bool operator==(const JSONTable& lhs, const JSONTable& rhs) {
    return lhs.mRoot == rhs.mRoot;
}

bool operator!=(const JSONTable& lhs, const JSONTable& rhs) {
    return !(lhs == rhs);
}
} // namespace logtail