#ifndef ARGUS_CLOUD_CLIENT_H
#define ARGUS_CLOUD_CLIENT_H

#include <chrono>
#include <utility>
#include <vector>
#include <list>

#include "common/ThreadWorker.h"
#include "common/FilePathUtils.h" // namespace fs

#include "base_collect.h"

namespace argus {
    struct CloudAgentInfo;
}

namespace common {
    struct HttpRequest;
}

namespace json{
    class Object;
}

struct ResourceConsumption;
struct tagThreadStack;

namespace cloudMonitor {

    struct ResourceWaterLevel {
        std::string name;
        double value = 0;
        double threshold = 0;
        int times = 0; // 超限次数

        ResourceWaterLevel() = default;
        ResourceWaterLevel(std::string s, double v, double t, int n):
            name(std::move(s)), value(v), threshold(t), times(n){}
    };

#include "common/test_support"
class CloudClient : public ThreadWorkerEx {
public:
    CloudClient();
    ~CloudClient() override;

    void Start();

    bool SendThreadsDump(const std::vector<ResourceWaterLevel> &vecResources,
                         const std::vector<ResourceConsumption> &topTasks
#if !defined(WITHOUT_MINI_DUMP)
                         , const std::list<tagThreadStack> &lstThreadStack
#endif
                         ) const;

private:
    bool SendHeartbeat(std::string &response);
    bool SendHeartbeat(const std::string &req, std::string &response);
    void DealHeartBeatResponse(const std::string &response);
    void GetHeartbeatJsonString(std::string &json) const;

    static void ParseHpcClusterConfig(const json::Object &);
    static void ParseFileStoreInfo(const json::Object &);

    static void InitPluginLib();
    void InitProxyManager() const;

private:
    static bool StoreFile(const fs::path &filePath, const std::string &content, const std::string &user);
    bool SendCoreDump() const;
    bool SaveDump(const std::string &type, const std::string &body) const;

    void CompleteHttpRequest(common::HttpRequest &request,
                             const std::string &uri,
                             const std::string &body,
                             const std::string &contentType = "text/plain") const;
private:
    std::chrono::seconds mInterval{180};
    std::chrono::seconds mFirstInterval{10};
    std::shared_ptr<argus::CloudAgentInfo> mCloudAgentInfo;
    size_t mTotalCount = 0;
    size_t mOkCount = 0;
    size_t mErrorCount = 0;
    size_t mContinueErrorCount = 0; //持续错误
    std::string mResponseMd5;
};
#include "common/test_support"

}
#endif 