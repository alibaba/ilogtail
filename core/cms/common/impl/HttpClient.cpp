#include "common/HttpClient.h"
#include "common/StringUtils.h"
#include "common/Logger.h"

#if !defined(DISABLE_BOOST_URL)
#include <boost/url.hpp>
#endif
#include <fmt/format.h>

#include "common/ThrowWithTrace.h"
#include "common/Chrono.h"

const char * const VPC_SERVER = "http://100.100.100.200";

#if !defined(DISABLE_BOOST_URL)
using namespace boost::urls;
#else

#endif

namespace common {
    std::string toString(HttpMethod method, bool raise) {
        switch (method) {
        case HttpMethod::GET:
            return "GET";
        case HttpMethod::POST:
            return "POST";
        case HttpMethod::HEAD:
            return "HEAD";
        default:
            if (!raise) {
                return fmt::format("UNKNOWN({})", static_cast<int>(method));
            }
            ThrowWithTrace<std::invalid_argument>("unknown HttpMethod: {}", static_cast<int>(method));
        }
    }

    std::string genUrl(const std::string &urlString, const std::map<std::string, std::string> &params) {
        std::string ret = TrimRight(urlString, "?");
        if (!params.empty() && !ret.empty()) {
#if !defined(DISABLE_BOOST_URL)
            // boost对以问号结尾的url处理不好，此处需预先trim掉。
            result<boost::url_view> r = parse_uri(ret);
            if (!r.has_error()) {
                url u{r.value()};
                for (const auto &it: params) {
                    u.params().append(param_view{it.first, it.second});
                }
                ret = u.buffer();
            }
#else
            if (!params.empty()) {
                bool question = (ret.find('?') != std::string::npos);
                ret += question? "": "?";
                const char *sep = (question? "&": "");
                for (const auto &pair: params) {
                    ret += sep + pair.first + "=" + HttpClient::UrlEncode(pair.second);
                    sep = "&";
                }
            }
#endif
        }
        return ret;
    }


#if !defined(DISABLE_BOOST_URL)
    std::string HttpClient::UrlEncode(const std::string &src) {
        return boost::urls::encode(src, boost::urls::unreserved_chars);
    }

    std::string HttpClient::UrlDecode(const std::string &src) {
        std::string ret;
        boost::urls::pct_string_view s = src;
        s.decode({}, boost::urls::string_token::assign_to(ret));
        return ret;
    }
#endif

    bool HttpClient::UrlParse(const std::string &url, std::string &hostStr, std::string &pathStr) {
#if !defined(DISABLE_BOOST_URL)
        using namespace boost::urls;
        result<boost::url_view> r = parse_uri(url);
        if (r.has_value() && !r.has_error()) {
            const url_view &u = r.value();
            hostStr = u.host();
            pathStr = u.path();
        }
#else
        std::string urlPath = url;
        size_t index = urlPath.find("://");
        if (index != std::string::npos) {
            urlPath = urlPath.substr(index + 3);
        }
        index = urlPath.find('/');
        if (index != std::string::npos) {
            hostStr = urlPath.substr(0, index);
            pathStr = urlPath.substr(index);
            index = pathStr.find('?');
            if (index != std::string::npos) {
                pathStr = pathStr.substr(0, index);
            }
        } else {
            hostStr = urlPath;
            pathStr = "";
        }
#endif
        return true;
    }

    /// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // HttpClient
    int HttpClient::performHttp(HttpMethod method, const HttpRequest& request, HttpResponse& response, const std::string& cipher) {
        try {
            return performHttpImpl(method, request, response, cipher);
        }
        catch (const std::exception& ex) {
            response.errorMsg = ex.what();
            LogWarn("[{}] {}, error: {}", toString(method), request.url, ex.what());
        }
        return -1;
    }

    std::string HttpClient::GetString(const std::string& url, const std::chrono::seconds &timeout) {
        HttpRequest httpRequest;
        httpRequest.url = url;
        httpRequest.timeout = ToSeconds(timeout);

        std::string content;

        HttpResponse httpResponse;
        int result = HttpClient::HttpCurl(httpRequest, httpResponse);
        bool success = (result == HttpClient::Success && httpResponse.resCode == 200);
        if (success) {
            content = httpResponse.result;
        }
        LogInfo("try to [GET]{} {}, curlCode: {}, responseCode: {}, errorMsg: {}, responseMsg: {}", httpRequest.url,
                (success ? "ok" : "error"), result, httpResponse.resCode, httpResponse.errorMsg, httpResponse.result);
        return content;
    }

    int HttpClient::HttpGet(const std::string &url, const std::map<std::string, std::string> &params,
                            HttpResponse &resp) {
        std::string fullUrl = genUrl(url, params);
        return HttpGet(fullUrl, resp);
    }

    int HttpClient::HttpGet(const std::string& url, HttpResponse& httpResponse) {
        HttpRequest request;
        request.timeout = 5;
        request.url = url;
        return performHttp(HttpMethod::GET, request, httpResponse);
    }

    int HttpClient::HttpPost(const HttpRequest& httpRequest, HttpResponse& httpResponse) {
        return performHttp(HttpMethod::POST, httpRequest, httpResponse);
    }

    int HttpClient::HttpCurl(const HttpRequest& httpRequest, HttpResponse& httpResponse) {
        return performHttp(HttpMethod::GET, httpRequest, httpResponse);
    }

    size_t HttpClient::HttpCallBack(void* buffer, size_t size, size_t num, void* p) {
        auto* p_httpResponse = (HttpResponse*)p;
        p_httpResponse->result.append((const char*)buffer, size * num);
        return size * num;
    }

    int HttpClient::HttpPostWithCipher(const HttpRequest& httpRequest, HttpResponse& httpResponse,
        const std::string& cipher) {
        return performHttp(HttpMethod::POST, httpRequest, httpResponse, cipher);
    }

    int HttpClient::HttpCurlWithCipher(const HttpRequest& httpRequest, HttpResponse& httpResponse,
        const std::string& cipher) {
        return performHttp(HttpMethod::GET, httpRequest, httpResponse, cipher);
    }

    int HttpClient::HttpHead(const HttpRequest& httpRequest, HttpResponse& httpResponse) {
        return performHttp(HttpMethod::HEAD, httpRequest, httpResponse);
    }

} // namespace common
