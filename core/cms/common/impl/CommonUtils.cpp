//
// Created by dezhi.wangdezhi on 12/27/17.
//
#ifdef WIN32
//#define WIN32_LEAN_AND_MEAN
// solve: use undefined symbol struct "sockaddr_in"
#   include <winsock2.h>
#   include <Windows.h>
#endif

#include "common/CommonUtils.h"

#include <boost/asio/ip/host_name.hpp>

#include "common/Uuid.h"
#include "common/TimeFormat.h"
#include "common/Chrono.h"

using namespace std;

namespace common {
    string CommonUtils::getHostName() {
        // const int maxHostNameLength = 128;
        // char hostName[maxHostNameLength]{ '\0' };
        // apr_gethostname(hostName, maxHostNameLength - 1, nullptr);
        // return string(hostName);
        boost::system::error_code ec;
        return boost::asio::ip::host_name(ec);
    }

    string CommonUtils::generateUuid() {
        return NewUuid();
    }

    string getTimeStrImpl(int64_t micros, std::string format, bool gmt) {
        if (gmt != (format.empty() || 'L' != *format.begin())) {
            if (gmt) {
                format = format.substr(1);
            } else {
                format = "L" + format;
            }
        }
        return date::format(std::chrono::FromMicros(micros), format.c_str());
    }

    string CommonUtils::GetTimeStr(int64_t micros,const string &format)
    {
        return getTimeStrImpl(micros, format, HasSuffix(format, "GMT"));
    }

    string CommonUtils::GetTimeStrGMT(int64_t micros, const char *format) {
        return getTimeStrImpl(micros, format, true);
    }
}
