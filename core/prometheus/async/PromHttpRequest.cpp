#include "prometheus/async/PromHttpRequest.h"

#include <cstdint>
#include <string>

#include "common/http/HttpRequest.h"
#include "prometheus/Mock.h"

namespace logtail {

PromHttpRequest::PromHttpRequest(const std::string& method,
                                 bool httpsFlag,
                                 const std::string& host,
                                 int32_t port,
                                 const std::string& url,
                                 const std::string& query,
                                 const std::map<std::string, std::string>& header,
                                 const std::string& body,
                                 std::shared_ptr<PromTaskFuture> event,
                                 uint64_t intervalSeconds,
                                 std::chrono::steady_clock::time_point execTime,
                                 std::shared_ptr<Timer> timer)
    : AsynHttpRequest(method, httpsFlag, host, port, url, query, header, body),
      mEvent(std::move(event)),
      mIntervalSeconds(intervalSeconds),
      mExecTime(execTime),
      mTimer(std::move(timer)) {
}

void PromHttpRequest::OnSendDone(const HttpResponse& response) {
    // ignore invalid state, just process the response
    mEvent->Process(response);
}

[[nodiscard]] bool PromHttpRequest::IsContextValid() const {
    return mEvent->IsCancelled();
}

} // namespace logtail