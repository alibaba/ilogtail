//
// Created by 韩呈杰 on 2023/5/9.
//

#ifndef ARGUSAGENT_BOOSTSTACKTRACEMT_H
#define ARGUSAGENT_BOOSTSTACKTRACEMT_H

#include <ostream>
#include <string>
#include <vector>
#include <boost/stacktrace.hpp>
#include <boost/stacktrace/frame.hpp>

// boost::stacktrace下的multi thread版
namespace boost { namespace stacktrace { namespace mt {
    // 线程安全
    std::string to_string(const std::vector<boost::stacktrace::frame> &frames);
    std::string to_string(const boost::stacktrace::stacktrace &bt);
}}}

std::ostream &operator<<(std::ostream &, const boost::stacktrace::stacktrace &);

#endif //ARGUSAGENT_BOOSTSTACKTRACEMT_H
