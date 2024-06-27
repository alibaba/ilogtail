#ifdef WIN32
#include <WinSock2.h> // depended by Ws2ipdef.h
#include <Ws2ipdef.h> // sockaddr_in6, Available in Windows Vista and later versions of the Windows operating systems.
#endif

#include "argus_manager.h"
#include "common/Config.h"
#include "common/Logger.h"

#include "common/ArgusMacros.h"
#ifdef ENABLE_CLOUD_MONITOR

#   if defined(ENABLE_FILE_CHANNEL)
#       include "FileChannel.h"
#   endif

#   include "cloudMonitor/cloud_client.h"
#   include "cloudMonitor/cloud_channel.h"

#else
#   include "LocalChannel.h"
#   include "FileChannel.h"
#   include "sls_channel.h"
#   include "loggerCollect/loki_channel.h"
// #ifndef WIN32
//     #include "SaChannle.h"
// #endif
#   include "shennong/ShennongChannel.h"
#   include "http_metric_collect.h"
#endif
#ifndef DISABLE_ALIMONITOR

#include "alimonitor/alimonitor_channel.h"

using namespace alimonitor;
#endif

#include "DomainSocketCollect.h"
#include "ScriptScheduler.h"
#include "ModuleScheduler2.h"
#include "task_monitor.h"
#include "ChannelManager.h"
#include "self_monitor.h"
#include "ExporterScheduler.h"
#include "TaskManager.h"

using namespace std;
using namespace common;

namespace argus {
    class ModuleItemsStore : public IDataStore<std::shared_ptr<std::map<std::string, ModuleItem>>> {
        std::weak_ptr<ModuleScheduler2> store;
    public:
        explicit ModuleItemsStore(const std::shared_ptr<ModuleScheduler2> &r) : store(r) {}

        void set(const std::shared_ptr<std::map<std::string, ModuleItem>> &r) override {
            if (auto l = store.lock()) {
                l->setItems(r);
            }
        }

        std::shared_ptr<std::map<std::string, ModuleItem>> get() const override {
            if (auto l = store.lock()) {
                return l->getItems();
            }
            return nullptr;
        }
    };

    class ExporterItemsStore : public IDataStore<std::shared_ptr<std::vector<ExporterItem>>> {
        std::weak_ptr<ExporterScheduler> store;
    public:
        explicit ExporterItemsStore(const std::shared_ptr<ExporterScheduler> &r) : store(r) {}

        void set(const std::shared_ptr<std::vector<ExporterItem>> &r) override {
            if (auto l = store.lock()) {
                l->setExporterItems(r);
            }
        }

        std::shared_ptr<std::vector<ExporterItem>> get() const override {
            if (auto l = store.lock()) {
                return l->getExporterItems();
            }
            return nullptr;
        }
    };
    class ShennongExporterItemsStore : public IDataStore<std::shared_ptr<std::vector<ExporterItem>>> {
        std::weak_ptr<ExporterScheduler> store;
    public:
        explicit ShennongExporterItemsStore(const std::shared_ptr<ExporterScheduler> &r) : store(r) {}

        void set(const std::shared_ptr<std::vector<ExporterItem>> &r) override {
            if (auto l = store.lock()) {
                l->setShennongExporterItems(r);
            }
        }

        std::shared_ptr<std::vector<ExporterItem>> get() const override {
            if (auto l = store.lock()) {
                return l->getShennongExporterItems();
            }
            return nullptr;
        }
    };

    ArgusManager::ArgusManager(TaskManager *tm) {
        mSelfMonitor = std::make_shared<SelfMonitor>();
        mChannelManager = std::make_shared<ChannelManager>();

        auto *cfg = SingletonConfig::Instance();
        if (cfg->GetValue("agent.enable.domainsocket.listen", true)) {
            mDomainSocketCollector = std::make_shared<DomainSocketCollect>();
            LogInfo("will enable DomainSocketCollect");
        }
        if (cfg->GetValue("agent.enable.baseMetric.collect", true)) {
            swap(std::make_shared<ModuleScheduler2>(), tm);
            LogInfo("will enable ModuleScheduler2");
        }
        if (cfg->GetValue("agent.enable.script.collect", true)) {
            swap(std::make_shared<ScriptScheduler>(), tm);
            LogInfo("will enable ScriptScheduler");
        }
        if (cfg->GetValue("agent.enable.exporter.collect", true)) {
            swap(std::make_shared<ExporterScheduler>(), tm);
            LogInfo("will enable ExporterScheduler");
        }
    }

    ArgusManager::~ArgusManager() = default;

    std::shared_ptr<OutputChannel> ArgusManager::GetOutputChannel(const std::string &name) const {
        return mChannelManager->GetOutputChannel(name);
    }

    template<typename T>
    void AddOutputChannel(const std::shared_ptr<ChannelManager> &chanManager,
                          const std::string &chanName, const std::string &cfgKey) {
        if (chanManager && (cfgKey.empty() || SingletonConfig::Instance()->GetValue(cfgKey, true))) {
            chanManager->AddOutputChannel(chanName, std::make_shared<T>());
            LogInfo("will enable output-channel: {}", chanName);
        }
    }

    void ArgusManager::Start(bool enableCloudClient) {
        mSelfMonitor->Start();
#ifdef ENABLE_CLOUD_MONITOR
        if (enableCloudClient) {
            auto cloudClient = std::make_shared<cloudMonitor::CloudClient>();
            cloudClient->Start(); // 由于要初始化代理，此处可能会很慢
            mCloudClient = cloudClient;
        }
        AddOutputChannel<cloudMonitor::CloudChannel>(mChannelManager, cloudMonitor::CloudChannel::Name, {});
#   ifdef ENABLE_FILE_CHANNEL
        AddOutputChannel<FileChannel>(mChannelManager, "localfile", "agent.enable.localfile.channel");
#   endif
#else
        AddOutputChannel<LocalChannel>(mChannelManager, "local", "agent.enable.local.channel");
        AddOutputChannel<FileChannel>(mChannelManager, "localfile", "agent.enable.localfile.channel");
        AddOutputChannel<SlsChannel>(mChannelManager, "sls", "agent.enable.sls.channel");
        AddOutputChannel<ShennongChannel>(mChannelManager, "shennong", "agent.enable.shennong.channel");
        AddOutputChannel<LokiChannel>(mChannelManager, "loki", "agent.enable.loki.channel");

        if(SingletonConfig::Instance()->GetValue("agent.enable.http.listen",  true)) {
            SingletonHttpMetricCollect::Instance()->Start();
            LogInfo("will enable http-listen");
        }
#endif

        if (mExporterScheduler) {
            mExporterScheduler->runIt();
        }
#ifndef DISABLE_ALIMONITOR
        AddOutputChannel<AlimonitorChannel>(mChannelManager, "alimonitor", "agent.enable.alimonitor.channel");
#endif
        mChannelManager->Start();  // 数据上报

        // 指标采集调度
        if (mModuleScheduler) {
            mModuleScheduler->runIt();
        }
        if (mScriptScheduler) {
            mScriptScheduler->runIt();
        }
        if (mDomainSocketCollector) {
            LogInfo("will enable base domainsocket listen");
            DomainSocketCollect::start(mDomainSocketCollector);
        }

        // 解析，各种配置，比如需要采集哪些指标等。
        SingletonTaskMonitor::Instance()->runIt();
    }

    template<typename T>
    static void collectStatus(const std::shared_ptr<T> &ptr, std::vector<CommonMetric> &metrics) {
        if (ptr) {
            CommonMetric moduleMetric;
            ptr->GetStatus(moduleMetric);
            metrics.push_back(moduleMetric);
        }
    }

    void ArgusManager::GetStatus(std::vector<CommonMetric> &metrics) const {
        metrics.clear();
        metrics.reserve(6);

        collectStatus(mModuleScheduler, metrics);
        collectStatus(mScriptScheduler, metrics);
        collectStatus(mExporterScheduler, metrics);

        auto collectChannel = [&](const char *channelName) {
            CommonMetric metric;
            if (mChannelManager->GetOutputChannelStatus(channelName, metric)) {
                metrics.push_back(metric);
            }
        };
        collectChannel("alimonitor");
        collectChannel("shennong");
        collectChannel("sls");
    }

    std::shared_ptr<ModuleScheduler2> ArgusManager::swap(const std::shared_ptr<ModuleScheduler2> &r, TaskManager *tm) {
        auto old = mModuleScheduler;
        mModuleScheduler = r;
        tm = (tm ? tm : SingletonTaskManager::Instance());
        tm->moduleItems.swap(std::make_shared<ModuleItemsStore>(mModuleScheduler));
        return old;
    }

    std::shared_ptr<ExporterScheduler> ArgusManager::swap(const std::shared_ptr<ExporterScheduler> &r, TaskManager *tm) {
        auto old = mExporterScheduler;
        mExporterScheduler = r;
        tm = (tm ? tm : SingletonTaskManager::Instance());
        tm->exporterItems.swap(std::make_shared<ExporterItemsStore>(mExporterScheduler));
        tm->shennongExporterItems.swap(std::make_shared<ShennongExporterItemsStore>(mExporterScheduler));
        return old;
    }

#define IMPLEMENT_SWAP(T)                                                               \
    std::shared_ptr<T> ArgusManager::swap(const std::shared_ptr<T> &r, TaskManager *) { \
        auto old = m ## T;                                                              \
        m ## T = r;                                                                     \
        return old;                                                                     \
    }

    // IMPLEMENT_SWAP(ExporterScheduler)

    IMPLEMENT_SWAP(ScriptScheduler)

#undef IMPLEMENT_SWAP

}
