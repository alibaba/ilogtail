#include "prometheus/async/PromHttpRequest.h"

#include <curl/curl.h>

#include <cstdint>
#include <string>

#include "common/http/HttpRequest.h"

namespace logtail {

PromHttpRequest::PromHttpRequest(const std::string& method,
                                 bool httpsFlag,
                                 const std::string& host,
                                 int32_t port,
                                 const std::string& url,
                                 const std::string& query,
                                 const std::map<std::string, std::string>& header,
                                 const std::string& body,
                                 uint32_t timeout,
                                 uint32_t maxTryCnt,
                                 bool followRedirects,
                                 std::shared_ptr<PromFuture> future)
    : AsynHttpRequest(method, httpsFlag, host, port, url, query, header, body, timeout, maxTryCnt),
      mFuture(std::move(future)),
      mFollowRedirects(followRedirects) {
}

void PromHttpRequest::SetAdditionalOptions(CURL* curl) const {
    if (curl) {
        // follow redirect
        if (mFollowRedirects) {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        }
        // gzip
        // curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        // tls
        // 设置 CA 证书
        // curl_easy_setopt(curl, CURLOPT_CAINFO, "/path/to/cacert.pem");

        // // 可能需要证书
        // curl_easy_setopt(curl, CURLOPT_SSLCERT, "/path/to/client-cert.pem");
        // curl_easy_setopt(curl, CURLOPT_SSLCERTPASSWD, "password");
    }
}

void PromHttpRequest::OnSendDone(const HttpResponse& response) {
    mFuture->Process(response, mLastSendTime * 1000);
}

[[nodiscard]] bool PromHttpRequest::IsContextValid() const {
    return true;
}

} // namespace logtail