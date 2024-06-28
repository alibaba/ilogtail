#ifndef ARGUSAGENT_SIC_THROW_WITH_TRACE_H
#define ARGUSAGENT_SIC_THROW_WITH_TRACE_H

#include <type_traits>
#include <boost/stacktrace.hpp>
#include <boost/exception/all.hpp>
#include <fmt/format.h>

typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace> traced;

template<typename E,
        typename std::enable_if<std::is_base_of<std::exception, E>::value, int>::type = 0>
[[noreturn]] void ThrowWithTrace(const E &e) {
    throw boost::enable_error_info(e) << traced(boost::stacktrace::stacktrace());
}

template<typename E,
        typename std::enable_if<std::is_base_of<std::exception, E>::value, int>::type = 0,
        typename ...Args>
[[noreturn]] void ThrowWithTrace(const char *errMsg, Args &&...args) {
    if (sizeof...(args) == 0) {
        throw boost::enable_error_info(E{errMsg}) << traced(boost::stacktrace::stacktrace());
    } else {
        throw boost::enable_error_info(E{fmt::format(errMsg, std::forward<Args>(args)...)})
                << traced(boost::stacktrace::stacktrace());
    }
}

template<typename E,
        typename std::enable_if<std::is_base_of<std::exception, E>::value, int>::type = 0,
        typename ...Args>
[[noreturn]] void ThrowWithTrace(const std::string &errMsg, Args &&...args) {
    if (sizeof...(args) == 0) {
        throw boost::enable_error_info(E{errMsg}) << traced(boost::stacktrace::stacktrace());
    } else {
        throw boost::enable_error_info(E{fmt::format(errMsg, std::forward<Args>(args)...)})
                << traced(boost::stacktrace::stacktrace());
    }
}

template<typename Excep,
        typename std::enable_if<std::is_base_of<std::exception, Excep>::value, int>::type = 0,
        typename ...Args>
[[noreturn]] void Throw(const std::string &errMsg, Args &&...args) {
    throw Excep{fmt::format(errMsg, std::forward<Args>(args)...)};
}

std::ostream &operator<<(std::ostream &, const std::exception &);

#endif //!ARGUSAGENT_SIC_THROW_WITH_TRACE_H
