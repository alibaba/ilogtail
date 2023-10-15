#include "config/NewConfig.h"

namespace logtail {
bool operator==(const NewConfig& lhs, const NewConfig &rhs) {
    return (lhs.mName == rhs.mName) && (*lhs.mDetail == *rhs.mDetail);
}

bool operator!=(const NewConfig& lhs, const NewConfig &rhs) {
    return !(lhs == rhs);
}
}