#pragma once
#include <cstdint>
#include <string>

#include "ScrapeConfig.h"
#include "common/http/HttpRequest.h"
#include "prometheus/async/PromFuture.h"


namespace logtail {

class PromHttpRequest : public AsynHttpRequest {
public:
    PromHttpRequest(const std::string& method,
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
                    std::shared_ptr<ScrapeConfig> scrapeConfig = nullptr);
    PromHttpRequest(const PromHttpRequest&) = default;
    ~PromHttpRequest() override = default;

    // void SetAdditionalOptions(CURL*) const override;

    void OnSendDone(const HttpResponse& response) override;
    [[nodiscard]] bool IsContextValid() const override;

private:
    void SetNextExecTime(std::chrono::steady_clock::time_point execTime);

    std::shared_ptr<PromFuture> mFuture;

    bool mFollowRedirects = false;
    std::shared_ptr<ScrapeConfig> mScrapeConfig;
};

} // namespace logtail