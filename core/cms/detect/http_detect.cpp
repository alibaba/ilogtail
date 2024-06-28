#include "http_detect.h"

#include "common/Logger.h"
#include "common/HttpClient.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"

using namespace std;
using namespace common;

namespace common {
    HttpDetectResult::HttpDetectResult() = default;

    HttpDetectResult::HttpDetectResult(int RspCode, size_t bodyLength) {
        code = RspCode;
        bodyLen = bodyLength;
    }

    HttpDetect::HttpDetect(const string &taskId,
                           const string &url,
                           const string &method,
                           const string &requestBody,
                           const string &header,
                           const string &keyword,
                           bool negative,
                           const std::chrono::seconds &timeout) {
        mTaskId = taskId;
        mUrl = url;
        mMethod = method;
        mHeaders = StringUtils::split(header, ",\n", true);
        mRequestBody = requestBody;
        mKeyword = keyword;
        mNegative = negative;
        mTimeout = timeout;
    }

    HttpDetectResult HttpDetect::Detect() const {
        constexpr bool implicitGet = true; // mMethod为空时，按GET处理

        int res = -1;
        HttpRequest request;
        HttpResponse response;
        request.httpHeader = mHeaders;
        request.body = mRequestBody;
        request.url = mUrl;
        request.timeout = static_cast<int>(ToSeconds(mTimeout));
        std::string method = implicitGet && mMethod.empty() ? "GET" : ToUpper(mMethod);
        if (method == "HEAD") {
            res = HttpClient::HttpHead(request, response);
        } else if (method == "POST") {
            res = HttpClient::HttpPost(request, response);
        } else if (method == "GET") {
            res = HttpClient::HttpCurl(request, response);
        } else {
            response.errorMsg = fmt::format("invalid method: {}", (mMethod.empty() ? "<nil>" : mMethod));
        }

        if (res == -1) {
            LogWarn("TaskID: {}, curl code: {}, curl error: {}", mTaskId, res, response.errorMsg);
            return {HTTP_FAIL, 0};
        } else if (res != HttpClient::Success) {
            LogWarn("TaskID: {}, curl code: {}, curl error: {}", mTaskId, res, response.errorMsg);
            return {(HttpClient::IsTimeout(res) ? HTTP_TIMEOUT : HTTP_FAIL), 0};
        }

        LogDebug("TaskID: {}, response code: {}, content: {}", mTaskId, response.resCode, mResponse);
        if (response.resCode < 200 || response.resCode >= 300 || mKeyword.empty()) {
            return {response.resCode, response.result.size()};
        }

        bool match = response.result.find(mKeyword) != string::npos;
        if (mNegative) {
            match = !match;
        }
        if (!match) {
            return {HTTP_MISMATCHING, response.result.size()};
        }
        return {response.resCode, response.result.size()};
    }

}