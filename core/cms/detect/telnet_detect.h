#ifndef _TELNET_DETECT_H_
#define _TELNET_DETECT_H_

#include <string>
#include <chrono>

namespace common {
#define TELNET_FAIL  631
#define TELNET_SUCCESS 200
#define TELNET_TIMEOUT  630
#define TELNET_MISMATCHING  635

#include "common/test_support"
class TelnetDetect {
public:
    explicit TelnetDetect(const std::string &uri,
                          const std::string &requestBody = "",
                          const std::string &keyword = "",
                          bool negative = false,
                          const std::chrono::seconds &timeout = std::chrono::seconds{3});

    ~TelnetDetect();

    int Detect();

private:

    void Init();

    static std::string ToUnixRequestBody(const std::string &body);

private:
    std::string mUri;
    std::string mHost;
    std::string mRequestBody;
    std::string mKeyword;
    uint16_t mPort = 23;
    bool mNegative = false;
    std::chrono::seconds mTimeout{3};
};
#include "common/test_support"

}
#endif