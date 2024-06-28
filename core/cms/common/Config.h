#ifndef ARGUSAGENT_MAIN_CONFIG_H
#define ARGUSAGENT_MAIN_CONFIG_H

#include "common/ConfigBase.h"

#include <string>
#include <set>
#include <vector>
#include <atomic>
#include <memory>
#include <chrono>

#include "common/FilePathUtils.h"
#include "common/Singleton.h"
#include "common/DefaultConfig.h"
#include "common/PropertiesFile.h"

#include "common/test_support"
class Config : public ConfigBase {
public:
    Config();

    ~Config() override;

    int loadConfig(const std::string &confFile);

    std::string getConfigString();

#if defined(ENABLE_ALIMONITOR)
    inline std::string getMainIp() const { return m_mainIp; }
#endif
    inline int getMaxProcNum() const { return m_config.maxProcNum; }

    inline auto getScheduleIntv() const -> decltype(common::ConfigProfile::scheduleIntv) {
        return m_config.scheduleIntv;
    }

    inline auto getMaxOutputLen() const -> decltype(common::ConfigProfile::maxOutputLen) {
        return m_config.maxOutputLen;
    }

    inline auto getScheduleFactor() const -> decltype(common::ConfigProfile::scheduleFactor){
        return m_config.scheduleFactor;
    }

#ifdef ENABLE_ALIMONITOR
    inline int getHttpListenPort() const { return m_config.httpListenerPort; }
    inline int getAppListenPort() const { return m_config.appListenerPort; }
    inline int getMaxMsgQueueSize() const { return m_config.maxMsgQueueSize; }

    inline auto getMaxRegisterIntv() const ->decltype(common::ConfigProfile::maxRegisterIntv) {
        return m_config.maxRegisterIntv;
    }

    std::vector<std::string> &getMasterHosts();
    int getMasterPort();
    inline int getMaxMemUse() const { return m_config.maxMemUse; }
    inline int getRcPort() const { return m_config.rcPort; }
    inline int getRcPortBackup() const { return m_config.rcBackupPort; }
    inline fs::path getScriptBaseDir() { return m_baseDir / "local_data" / "libexec"; }
    inline int getHttpScriptRunListenPort() const { return m_config.httpScriptRunListenerPort; }
    std::set<std::string> getLocalIps();
#endif
    // inline auto getModuleScheduleIntv() const -> decltype(common::ConfigProfile::mModuleScheduleIntv) {
    //     return m_config.mModuleScheduleIntv;
    // }

    inline std::string &getLocalhost() { return m_config.localhost; }

    VIRTUAL int getInnerListenPort();

    inline fs::path getInnerDomainSocketPath() const {
        return m_baseDir / m_config.mInnerSocketPath;
    }

    inline std::string getReceiveDomainSocketPath() const {
        return m_config.mReceiveDomainSocketPath;
    }

    inline int getReceivePort() const {
        return m_config.mReceivePort;
    }

    int getModuleThreadNum() const { return m_config.moduleCollectThreadNum; }

    int getMaxModuleThreadNum() const { return m_config.maxModuleCollectThreadNum; }

	inline fs::path getLogDir() const { return m_baseDir / "local_data" / "logs"; }

    inline fs::path getLogFile() const { return getLogDir() / "argus.log"; }

    inline fs::path getModuleBaseDir() const { return m_baseDir / "module"; }

    inline fs::path getModuleDataDir() const { return m_baseDir / "local_data" / "data"; }

    inline void setModuleTaskFile(const std::string &taskFile) { m_moduleTaskFile = taskFile; }

    inline void getModuleTaskFile(std::string &taskFile) { taskFile = m_moduleTaskFile; }

    inline void setScriptTaskFile(const std::string &taskFile) { m_scriptTaskFile = taskFile; }

    inline void getScriptTaskFile(std::string &taskFile) { taskFile = m_scriptTaskFile; }

    inline void setReceiveTaskFile(const std::string &taskFile) { m_receiveTaskFile = taskFile; }

    inline void getReceiveTaskFile(std::string &taskFile) { taskFile = m_receiveTaskFile; }

    inline const fs::path &getBaseDir() const { return m_baseDir; }

    fs::path setBaseDir(const fs::path& baseDir);

    const std::string &getEnvPath() const { return m_config.envPath; }

    using ConfigBase::GetValue;
    std::string GetValue(const std::string &key, const std::string &defaultValue) const override;

    /** 神农相关配置 **/
    bool GetIsTianjiEnv() const;

    void SetIsTianjiEnv(bool const &isTianjiEnv);

    bool GetShennongOn() const;

    void SetShennongOn(bool const &isShennongOn);
#ifdef ENABLE_COVERAGE
    void Set(const std::string &key, const std::string &v);
#endif
private:
    common::ConfigProfile m_config;
    //other
    fs::path m_baseDir;
#ifdef ENABLE_ALIMONITOR
    std::set<std::string> m_localIps;
    std::string m_mainIp;
#endif
    std::string m_moduleTaskFile;
    std::string m_scriptTaskFile;
    std::string m_receiveTaskFile;
    std::shared_ptr<common::PropertiesFile> mPropertiesFile;

    /** 神农相关配置 **/

    /**
     * 是否为天基环境
     */
    std::atomic<bool> mIsTianjiEnv{false};

    std::atomic<bool> mShennongOn{false};

};
#include "common/test_support"

typedef common::Singleton<Config> SingletonConfig;

#endif
