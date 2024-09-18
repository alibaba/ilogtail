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
                                 std::shared_ptr<PromFuture> future,
                                 std::shared_ptr<ScrapeConfig> scrapeConfig)
    : AsynHttpRequest(method, httpsFlag, host, port, url, query, header, body, timeout, maxTryCnt),
      mFuture(std::move(future)),
      mFollowRedirects(followRedirects),
      mScrapeConfig(std::move(scrapeConfig)) {
}

void SetAdditionalOptions(CURL* curl,
                          bool followRedirects,
                          const std::string& acceptEncoding,
                          const std::string& caFile,
                          const std::string& certFile,
                          const std::string& keyFile,
                          bool insecureSkip,
                          const std::string& proxyURL,
                          const std::string& noProxy) {
    if (curl) {
        // follow redirect
        if (followRedirects) {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        }

        // encoding
        if (!acceptEncoding.empty()) {
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, acceptEncoding.c_str());
        }

        // tls
        if (!caFile.empty()) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, caFile.c_str());
        }
        if (!certFile.empty()) {
            curl_easy_setopt(curl, CURLOPT_SSLCERT, certFile.c_str());
        }
        if (!keyFile.empty()) {
            curl_easy_setopt(curl, CURLOPT_SSLKEY, keyFile.c_str());
        }
        if (insecureSkip) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        } else {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        }

        // proxy
        if (!proxyURL.empty()) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        }
        if (!noProxy.empty()) {
            curl_easy_setopt(curl, CURLOPT_NOPROXY, noProxy.c_str());
        }
    }
}

void PromHttpRequest::OnSendDone(const HttpResponse& response) {
    mFuture->Process(response, mLastSendTime * 1000);
}

[[nodiscard]] bool PromHttpRequest::IsContextValid() const {
    return true;
}

} // namespace logtail