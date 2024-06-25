//
// Created by 韩呈杰 on 2023/5/9.
//
#include "common/BoostStacktraceMt.h"

#include <iomanip>
#include <sstream>
#include <mutex>

namespace boost { namespace stacktrace { namespace mt {
#ifndef WIN32
    static std::recursive_mutex stacktraceImplMutex;
    // 这个类析构时不回收backtrace的内存，多实例时会有内存泄漏
    static boost::stacktrace::detail::to_string_impl *pimpl = nullptr;

    std::string to_string(const std::vector<boost::stacktrace::frame> &frames) {
        std::stringstream ss;
        if (!frames.empty()) {
            std::lock_guard<std::recursive_mutex> guard(stacktraceImplMutex);
            if (pimpl == nullptr) {
                pimpl = new boost::stacktrace::detail::to_string_impl();
            }
            boost::stacktrace::detail::to_string_impl &impl = *pimpl;

            const size_t size = frames.size();
            for (std::size_t i = 0; i < size; ++i) {
                ss << std::setw(2) << std::setfill('0') << i;
                ss << "# ";
                ss << impl(frames[i].address());
                ss << std::endl;
            }
        }
        return ss.str();
    }

    std::string to_string(const boost::stacktrace::stacktrace &bt) {
        return to_string(bt.as_vector());
    }
#else
    std::string to_string(const std::vector<boost::stacktrace::frame> &frames) {
        if (frames.empty()) {
            return {};
        }
        return boost::stacktrace::detail::to_string(&frames[0], frames.size());
    }

    std::string to_string(const boost::stacktrace::stacktrace &bt) {
        return boost::stacktrace::to_string(bt);
    }
#endif
}}}

std::ostream &operator<<(std::ostream &os, const boost::stacktrace::stacktrace &bt) {
    os << boost::stacktrace::mt::to_string(bt);
    return os;
}
