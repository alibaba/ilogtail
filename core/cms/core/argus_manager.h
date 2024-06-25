#ifndef ARGUS_CORE_ARGUS_MANAGER_H
#define ARGUS_CORE_ARGUS_MANAGER_H

#include <utility>
#include <memory>

#include "ChannelManager.h"

#include "common/CommonMetric.h"
#include "common/Singleton.h"

#ifdef ENABLE_CLOUD_MONITOR
namespace cloudMonitor {
    class CloudClient;
}
#endif

namespace argus {
    class ModuleScheduler2;
    class ScriptScheduler;
    class ExporterScheduler;
    class SelfMonitor;
    class OutputChannel;
    class DomainSocketCollect;
    class TaskManager;

#include "common/test_support"
    class ArgusManager {
    public:
        explicit ArgusManager(TaskManager * = nullptr);
        ~ArgusManager();
        void Start();

        std::shared_ptr<ModuleScheduler2> getModuleScheduler() const { return mModuleScheduler; }
        std::shared_ptr<ScriptScheduler> getScriptScheduler() const { return mScriptScheduler; }
        std::shared_ptr<ChannelManager> getChannelManager() const { return mChannelManager; }
        std::shared_ptr<ExporterScheduler> getExporterScheduler() const { return mExporterScheduler; }
#ifdef ENABLE_CLOUD_MONITOR
        std::shared_ptr<cloudMonitor::CloudClient> GetCloudClient() const { return mCloudClient; }
#endif

        std::shared_ptr<OutputChannel> GetOutputChannel(const std::string &name) const;

        template<typename TChannel>
        std::shared_ptr<TChannel> GetChannel(const std::string &name) const {
            return mChannelManager->Get<TChannel>(name);
        }

        void GetStatus(std::vector<common::CommonMetric> &metrics) const;

#ifdef ENABLE_COVERAGE
    public:
#else
    private:
#endif
        std::shared_ptr<ModuleScheduler2> swap(const std::shared_ptr<ModuleScheduler2> &, TaskManager * = nullptr);
        std::shared_ptr<ExporterScheduler> swap(const std::shared_ptr<ExporterScheduler> &, TaskManager * = nullptr);
        std::shared_ptr<ScriptScheduler> swap(const std::shared_ptr<ScriptScheduler> &, TaskManager * = nullptr);

    private:
        std::shared_ptr<DomainSocketCollect> mDomainSocketCollector;
        std::shared_ptr<ModuleScheduler2> mModuleScheduler;
        std::shared_ptr<ScriptScheduler> mScriptScheduler;
        std::shared_ptr<ExporterScheduler> mExporterScheduler;
        std::shared_ptr<ChannelManager> mChannelManager;
        std::shared_ptr<SelfMonitor> mSelfMonitor;
#ifdef ENABLE_CLOUD_MONITOR
        std::shared_ptr<cloudMonitor::CloudClient> mCloudClient;
#endif
    };
#include "common/test_support"

    typedef common::Singleton<ArgusManager> SingletonArgusManager;
}
#endif
