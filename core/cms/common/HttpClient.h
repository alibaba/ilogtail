#ifndef ARGUS_COMMON_HTTP_CLIENT_H
#define ARGUS_COMMON_HTTP_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <chrono>

extern const char * const VPC_SERVER;

enum class HttpStatus {
    OK = 200,
    NotFound = 404,
};

namespace common {
    struct HttpRequest {
        std::string url;
        std::string body;
        std::string unixSocketPath;
        int timeout = 5; // 单位：秒
        std::string proxy;
        std::string proxySchemeVersion; // 仅当proxy为https时有效，目前仅支持"2" -> https/2
        std::string user;
        std::string password;
        std::vector<std::string> httpHeader;
    };

    struct HttpResponse {
        std::string contentEncoding;
        std::string result;
        std::string errorMsg;
        int resCode = 0;  // <=0 : 网络交互失败
    };

    enum class HttpMethod {
        GET,
        POST,
        HEAD,
    };
    std::string toString(HttpMethod method, bool raise = false);

    std::string genUrl(const std::string &url, const std::map<std::string, std::string>& params);

    class HttpClient {
    public :
        enum {
            Success = 0,
        };
        static bool IsTimeout(int resCode);
        static std::string GetString(const std::string &url, const std::chrono::seconds &timeout);
        static int HttpGet(const std::string &url, HttpResponse &httpResponse);
        static int HttpGet(const std::string &url,
                           const std::map<std::string, std::string> &params,
                           HttpResponse &httpResponse);
        static int HttpPost(const HttpRequest &httpRequest, HttpResponse &httpResponse);
        static int HttpPostWithCipher(const HttpRequest &httpRequest,
                                      HttpResponse &httpResponse,
                                      const std::string &cipher);
        static int HttpCurl(const HttpRequest &httpRequest, HttpResponse &httpResponse);
        static int HttpCurlWithCipher(const HttpRequest &httpRequest,
                                      HttpResponse &httpResponse,
                                      const std::string &cipher);
        static int HttpHead(const HttpRequest &httpRequest, HttpResponse &httpResponse);

        static size_t HttpCallBack(void *buffer, size_t size, size_t num, void *p);
        static std::string UrlEncode(const std::string &src);
        static std::string UrlDecode(const std::string &src);
        static bool UrlParse(const std::string &url, std::string &hostStr, std::string &pathStr);

        static std::string StrError(int code);
    private:
		static int performHttp(HttpMethod method,
			const HttpRequest& request,
			HttpResponse& response,
			const std::string& cipher = {});

        static int performHttpImpl(HttpMethod method,
            const HttpRequest& request,
            HttpResponse& response,
            const std::string& cipher = {},
            const std::string& netInterface = {});
    };
}
#endif // !ARGUS_COMMON_HTTP_CLIENT_H
