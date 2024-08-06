#include "prometheus/async/PromHttpRequest.h"

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
                                 std::shared_ptr<PromFuture> future)
    : AsynHttpRequest(method, httpsFlag, host, port, url, query, header, body), mFuture(std::move(future)) {
}

void PromHttpRequest::OnSendDone(const HttpResponse& response) {
    mFuture->Process(response);
}

[[nodiscard]] bool PromHttpRequest::IsContextValid() const {
    return mFuture->IsCancelled();
}

} // namespace logtail