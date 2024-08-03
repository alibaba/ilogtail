#pragma once

#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <queue>
#include <string>


namespace logtail {

bool caseInsensitiveComp(const char lhs, const char rhs);

bool compareHeader(const std::string& lhs, const std::string& rhs);

struct HttpResponse {
    int32_t mStatusCode = 0; // 0 means no response from server
    std::map<std::string, std::string, decltype(compareHeader)*> mHeader;
    std::string mBody;

    HttpResponse() : mHeader(compareHeader) {}
};

struct HttpRequest {
    std::string mMethod;
    bool mHTTPSFlag = false;
    std::string mUrl;
    std::string mQueryString;

    std::map<std::string, std::string> mHeader;
    std::string mBody;
    std::string mHost;

    uint32_t mTryCnt = 1;
    time_t mLastSendTime = 0;

    HttpRequest(const std::string& method,
                bool httpsFlag,
                const std::string& host,
                const std::string& url,
                const std::string& query,
                const std::map<std::string, std::string>& header,
                const std::string& body)
        : mMethod(method),
          mHTTPSFlag(httpsFlag),
          mUrl(url),
          mQueryString(query),
          mHeader(header),
          mBody(body),
          mHost(host) {}
    virtual ~HttpRequest() = default;
};

struct AsynHttpRequest : public HttpRequest {
    HttpResponse mResponse;
    void* mPrivateData = nullptr;
    time_t mEnqueTime = 0;

    AsynHttpRequest(const std::string& method,
                    bool httpsFlag,
                    const std::string& host,
                    const std::string& url,
                    const std::string& query,
                    const std::map<std::string, std::string>& header,
                    const std::string& body)
        : HttpRequest(method, httpsFlag, host, url, query, header, body) {}

    virtual bool IsContextValid() const = 0;
    virtual void OnSendDone(const HttpResponse& response) = 0;
};

class TimerEvent {
    friend bool operator<(const TimerEvent& lhs, const TimerEvent& rhs);

public:
    virtual ~TimerEvent() = default;

    virtual bool IsValid() const = 0;
    virtual bool Execute() = 0;

    std::chrono::steady_clock::time_point GetExecTime() const { return mExecTime; }

private:
    std::chrono::steady_clock::time_point mExecTime;
};

bool operator<(const TimerEvent& lhs, const TimerEvent& rhs) {
    return lhs.mExecTime < rhs.mExecTime;
}

class Timer {
public:
    void Init();
    void Stop();
    void PushEvent(std::unique_ptr<TimerEvent>&& e);

private:
    void Run();

    mutable std::mutex mQueueMux;
    std::priority_queue<std::unique_ptr<TimerEvent>> mQueue;

    std::future<void> mThreadRes;
    mutable std::mutex mThreadRunningMux;
    bool mIsThreadRunning = true;
    mutable std::condition_variable mCV;
};

class HttpRequestTimerEvent : public TimerEvent {
public:
    bool IsValid() const override;
    bool Execute() override;

    HttpRequestTimerEvent(std::unique_ptr<AsynHttpRequest>&& request) : mRequest(std::move(request)) {}

private:
    std::unique_ptr<AsynHttpRequest> mRequest;
};

} // namespace logtail