#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif
#include "telnet_detect.h"

#include <boost/algorithm/string/replace.hpp>

#include "common/NetWorker.h"
#include "common/Logger.h"
#include "common/Common.h"
#include "common/StringUtils.h"

#include <apr-1/apr_network_io.h>

using namespace std;

namespace common {
    TelnetDetect::TelnetDetect(const string &uri,
                               const string &requestBody,
                               const string &keyword,
                               bool negative,
                               const std::chrono::seconds &timeout) {
        mUri = uri;
        mRequestBody = requestBody;
        mKeyword = keyword;
        mNegative = negative;
        mTimeout = timeout;
        Init();
    }

    TelnetDetect::~TelnetDetect() = default;

    void TelnetDetect::Init() {
        std::string uri = (!mHost.empty() ? mHost : mUri);
        //解析uri
        size_t index = uri.find("://");
        if (index != string::npos) {
            uri = uri.substr(index + 3);
        }
        index = uri.find(':');
        if (index != string::npos) {
            mHost = uri.substr(0, index);
            mPort = convert<uint16_t>(uri.substr(index + 1));
        } else {
            mHost = uri;
            mPort = 23;
        }
        // if (mNetWorker.socket() != APR_SUCCESS) {
        //     return false;
        // }
        // if (mNetWorker.createAddr(&mDestAddr, mHost.c_str(), mPort) != APR_SUCCESS) {
        //     return false;
        // }
        // mNetWorker.setSockTimeOut(mTimeout);
        if (!mRequestBody.empty()) {
            mRequestBody = ToUnixRequestBody(mRequestBody);
        }
        LogDebug("TELNET: {}, port: {}, timeout: {}", mHost, mPort, mTimeout);
    }

    static int ReadLine(NetWorker &net, string &line) {
        // apr_socket_t *sock = net.getSock();
        line.reserve(1024);

        for (char buf = '\0'; buf != '\n';) {
            buf = '\0';
            size_t recvLen = 1;
			// apr_status_t rv = apr_socket_recv(sock, &buf, &recvLen);
            apr_status_t rv = net.recv(&buf, recvLen, false);
            if (recvLen <= 0 && APR_EAGAIN != rv) {
                LogDebug("read telnet response from {} error: ({}){}", net.remote(), rv, NetWorker::ErrorString(rv));
                return APR_STATUS_IS_TIMEUP(rv) ? TELNET_TIMEOUT : TELNET_FAIL;
            }
            if (recvLen == 1) {
                line.append(&buf, 1);
            }
        }
        return TELNET_SUCCESS;
    }

    int TelnetDetect::Detect() {
        NetWorker netWorker(fmt::format("TELNET({}:{})", mHost, mPort));
        netWorker.setTimeout(mTimeout);
        apr_status_t rv = netWorker.connect<TCP>(mHost, mPort, true);
        if (rv != APR_SUCCESS) {
            // LogDebug("mNetWorker.connect({}:{}) => ({}){}", mHost, mPort, rv, (rv == APR_TIMEUP? "(Timeout)": ""));
            return APR_STATUS_IS_TIMEUP(rv)? TELNET_TIMEOUT: TELNET_FAIL;
        }
        if (!mRequestBody.empty()) {
            size_t sendLen = mRequestBody.size();
            rv = netWorker.send(mRequestBody.c_str(), sendLen);
            if (rv != APR_SUCCESS) {
                LogWarn("Telnet[{}:{}], send request body {} bytes error: ({}){}",
                        mHost, mPort, mRequestBody.size(), rv, getErrorString(rv));
            }
        }
        if (mKeyword.empty()) {
            return TELNET_SUCCESS;
        }

        bool match = false;
        int ret = TELNET_SUCCESS;
        for (int i = 0; i < 30 && !match && ret == TELNET_SUCCESS; i++) {
            string line;
            ret = ReadLine(netWorker, line);
            if (ret == TELNET_SUCCESS) {
                match = (line.find(mKeyword) != string::npos);
            }
        }
        if (mNegative) {
            match = !match;
        }
        return match ? TELNET_SUCCESS : TELNET_MISMATCHING;
    }

    string TelnetDetect::ToUnixRequestBody(const string &body) {
        return boost::replace_all_copy(body, "\n", "\r\n");
    }
} // namespace common