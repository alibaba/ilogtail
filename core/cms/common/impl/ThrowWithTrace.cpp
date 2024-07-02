//
// Created by 韩呈杰 on 2023/7/22.
//
#include "common/ThrowWithTrace.h"
#include "common/BoostStacktraceMt.h"

std::ostream &operator<<(std::ostream &os, const std::exception &ex) {
    os << ex.what();
    const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(ex);
    if (st) {
        os << ", stacktrace:" << std::endl << boost::stacktrace::mt::to_string(*st);
    }
    os << std::endl;
    return os;
}
