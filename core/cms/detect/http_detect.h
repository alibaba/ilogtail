#ifndef ARGUS_HTTP_DETECT_H
#define ARGUS_HTTP_DETECT_H

#include <string>
#include <chrono>
#include <vector>

namespace common {
#define HTTP_FAIL  611
#define HTTP_TIMEOUT   610
#define HTTP_MISMATCHING  615

    struct HttpDetectResult {
        int code = HTTP_FAIL;
        size_t bodyLen = 0;

        HttpDetectResult();
        HttpDetectResult(int RspCode, size_t bodyLength);
    };

#include "common/test_support"
class HttpDetect {
public:
    HttpDetect(const std::string &taskId,
               const std::string &url,
               const std::string &method = "GET",
               const std::string &requestBody = "",
               const std::string &header = "",
               const std::string &keyword = "",
               bool negative = false,
               const std::chrono::seconds &timeout = std::chrono::seconds{10});

    ~HttpDetect() = default;

    HttpDetectResult Detect() const;

private:
    std::string mTaskId;
    std::string mUrl;
    std::string mMethod;
    std::string mKeyword;
    std::vector<std::string> mHeaders;
    std::string mRequestBody;
    std::string mResponse;
    bool mNegative = false;
    std::chrono::seconds mTimeout{10};
};
#include "common/test_support"

}
#endif 