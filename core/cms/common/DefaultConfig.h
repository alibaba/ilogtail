#ifndef ARGUS_COMMON_DEFAULT_CONFIG_H
#define ARGUS_COMMON_DEFAULT_CONFIG_H

#include <vector>
#include <string>
#include <chrono>
#include "Singleton.h"

namespace common {

    class ConfigProfile {
    public:
        ConfigProfile();
        void setDefault();

    public:
#ifdef ENABLE_ALIMONITOR
        //AliMaster的配置
        std::vector<std::string> masterHostVector;
        int masterPort;
        //app监听，用于jmx等app接入
        int appListenerPort;
        //rapter数据接口
        int httpScriptRunListenerPort;
        //脚本下载使用的端口
        int rcPort;
        int rcBackupPort;
        int maxMemUse;
        std::chrono::seconds maxRegisterIntv;
        int maxMsgQueueSize;
        //http监听，用于外部数据的接入，对应于PassiveItem
        int httpListenerPort;
#endif

        //公用的配置(Argus和Alimonitor都需要)
        std::chrono::microseconds scheduleIntv;
        std::chrono::seconds scheduleFactor;
        int maxProcNum;
        size_t maxOutputLen; // 单次最大上报数据的大小, 单位: byte

        //Argus调度相关的配置
        // std::chrono::microseconds mModuleScheduleIntv;
        // int maxModuleOutputLen;
        std::string envPath;
        int moduleCollectThreadNum;
        int maxModuleCollectThreadNum;
        //内部监听，linux为domain-socket，windows为tcp，监听127.0.0.1
        std::string localhost;
        int innerListenerPort;
        std::string mInnerSocketPath;
        //外部数据接入
        std::string mReceiveDomainSocketPath;
        int mReceivePort;
    };

#include "test_support"
    class DefaultConfig {
    public:
        DefaultConfig();

        ConfigProfile &getConfigProfile(const std::string &profile);

    private:
        ConfigProfile m_default;
        ConfigProfile m_nolog;
        ConfigProfile m_super;
        ConfigProfile m_test;
        ConfigProfile m_user; //user-defined
    };
#include "test_support"

    typedef Singleton<DefaultConfig> SingletonDefaultConfig;

}

#endif
