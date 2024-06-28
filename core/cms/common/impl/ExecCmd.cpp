//
// Created by 韩呈杰 on 2024/5/8.
//
#include "common/ExecCmd.h"
#include "common/StringUtils.h"
#include "common/Defer.h"
#include "common/TimeProfile.h"
#include "common/Logger.h"

#include <cstdio> // fileno

#include <boost/iostreams/stream.hpp>                 // 读取popen的输出
#include <boost/iostreams/device/file_descriptor.hpp> // 读取popen的输出

// boost::process v1封装太过复杂，且不支持带有管道的命令
// 同时boost::process还依赖了asio
// #define EXEC_CMD_USE_BOOST
#ifdef EXEC_CMD_USE_BOOST
#   include <boost/process.hpp>
#endif

#ifdef WIN32
#include <Windows.h>
#include <locale> // std::wstring_convert, std::codecvt
#include <codecvt>
#include <TlHelp32.h>

#   ifndef EXEC_CMD_USE_BOOST
#       include <process.h>
#       define popen _popen
#       define pclose _pclose
#   endif
#	ifndef fileno
#		define fileno _fileno
#	endif
#else

#include <unistd.h>
#include <sys/wait.h>  // WEXITSTATUS

#endif

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
tagExecCmdOption::tagExecCmdOption() {
    fnTrim = TrimSpace;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
int ExecCmd(const std::string &cmd, const std::function<void(const std::string &)> &append) {
    const char *funcName = __FUNCTION__;
    int ret = 0;
    TimeProfile tp;
    defer(
        auto cost = tp.cost();
        if (cost > std::chrono::milliseconds{ 500 }) {
            LogInfo("{}({}) => {}, cost: {}", funcName, cmd, ret, cost);
        }
    );

    // boost::process不支持这样的命令: ls -l > /tmp/ls.txt。
#ifndef EXEC_CMD_USE_BOOST
    FILE *fp = popen(cmd.c_str(), "r");
    if (fp != nullptr) {
#if 1
        namespace io = boost::iostreams;
        // 注意：这里只能使用file_descriptor_source，而不是file_descriptor_sink
        io::stream_buffer<io::file_descriptor_source> stream_buffer(fileno(fp), io::never_close_handle);
        std::istream ips(&stream_buffer);

        std::string line;
        while (std::getline(ips, line)) {
            append(line);
        }
#else
        std::stringstream ss;
        char buf[1024];
        size_t readLen;
        while ((readLen = fread(buf, 1, sizeof(buf), fp)) > 0) {
            ss.write(buf, static_cast<std::streamsize>(readLen));
        }
        if (feof(fp)) {
            append(ss.str());
        }
#endif

        ret = pclose(fp);
#ifndef WIN32
        ret = WEXITSTATUS(ret);
#endif
    }
#else
    namespace bp = boost::process;

    std::error_code ec;
    bp::ipstream ips; // reading pipe-stream
    bp::child c(cmd, bp::std_out > ips, bp::std_err > bp::null, ec);

    std::string line;
    while (c.running() && std::getline(ips, line)) {
        append(line);
    }

    c.wait();

    ret = c.exit_code(); // ec.value();
#endif
    return ret;
}

int ExecCmd(const std::string &cmd, std::string *out, const tagExecCmdOption *option) {
    std::function<void(const std::string &)> append = [&](const std::string &) {};
    if (out != nullptr) {
        out->clear();
        const char *crlf = "";
        append = [&](const std::string &r) {
            out->append(crlf).append(r);
            crlf = "\n";
        };
    }

    int ret = ExecCmd(cmd, append);

    tagExecCmdOption defaultOption;
    option = (option == nullptr ? &defaultOption : option);
    if (option->fnTrim && ret == 0 && out != nullptr) {
        *out = option->fnTrim(*out);
    }

    return ret;
}

int ExecCmd(const std::string &cmd, std::vector<std::string> &out, bool enableEmptyLine) {
    auto append = [&](const std::string &line) {
        if (enableEmptyLine || !line.empty()) {
            out.push_back(line);
        }
    };
    out.clear();

    return ExecCmd(cmd, append);
}
