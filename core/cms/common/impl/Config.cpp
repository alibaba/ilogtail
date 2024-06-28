#include "common/Config.h"

#include <vector>
#include <memory>

//#define LOG_STACKTRACE
#ifdef LOG_STACKTRACE
#include "common/boost_stacktrace_mt.h"
#endif

#include "common/Logger.h"
#include "common/Common.h"
#include "common/SystemUtil.h"
#include "common/JsonValueEx.h"
#include "common/FileUtils.h"
#include "common/Chrono.h"

using namespace std;
using namespace common;

Config::Config()
{
    m_config = SingletonDefaultConfig::Instance()->getConfigProfile(PROFILE);
    setBaseDir(::GetBaseDir());
#if defined(ENABLE_ALIMONITOR)
    m_mainIp = SystemUtil::getMainIp();
    LogInfo("init local main ip: {}", m_mainIp);
#endif
}

Config::~Config() = default;

fs::path Config::setBaseDir(const fs::path &baseDir) {
#ifdef LOG_STACKTRACE
    using namespace boost::stacktrace;
    LogInfo("BaseDirectory changed:\nFrom: {}\nTo  : {}\nCallStack: {}",
        m_baseDir.string(), baseDir.string(), mt::to_string(stacktrace()));
#else
    LogInfo("BaseDirectory changed:\nFrom: {}\nTo  : {}", m_baseDir.string(), baseDir.string());
#endif
    fs::path tmp = m_baseDir;
    m_baseDir = baseDir;
    return tmp;
}

int Config::loadConfig(const string& confFile)
{
    mPropertiesFile = std::make_shared<PropertiesFile>(confFile);
    // 专有云下编排样例(From越哥): https://code.alibaba-inc.com/apsara_stack/service-tianji/blob/v3.18xR/tianji_templates/TMPL-TIANJI-SERVICE-APSARASTACK-V3-PRODUCTION/user/ArgusAgent/agent.properties
    // 专有云PAI的特殊需求，专有云下无法渲染.properties文件，因此使用一个等效的json来做为补充
    const std::string suffix = ".properties";
    if (HasSuffix(confFile, suffix)) {
        std::string jsonFile = confFile.substr(0, confFile.size() - suffix.size()).append(".json");
        if (fs::exists(jsonFile)) {
            mPropertiesFile->AdjustPropertiesByJsonFile(jsonFile);
        }
    }
    fs::path persistentFile = m_baseDir / "local_data" / "conf" / "persistent_agent.properties";
    if (fs::exists(persistentFile)) {
        mPropertiesFile->AdjustPropertiesByFile(persistentFile);
    }
    m_config.maxOutputLen = this->GetValue("agent.logger.maxOutputLen", m_config.maxOutputLen);
    return 0;
}
std::string Config::getConfigString()
{
    boost::json::object body;
    body.reserve(8);
    // JsonValue jv;
    // Json::Value& body = jv.getValue()[(unsigned int)0];

    //DragoonMaster
    // vector<string> hosts = getMasterHosts();
    // for (Json::ArrayIndex i = 0; i < hosts.size(); i++)
    // {
    //     body["AliMaster"]["Host"][i] = hosts[i];
    // }
#ifdef ENABLE_ALIMONITOR
    boost::json::array arr;
    for (const auto &it: getMasterHosts()) {
        arr.emplace_back(it);
    }
    body["AliMaster"] = boost::json::object{
        {"Host", arr},
        {"Port", getMasterPort()},
    };
#endif
    //Common
    // //body["Common"]["LogLevel"] = getLogLevel();
    // body["Common"]["LogFile"] = getLogFile().string();
    // body["Common"]["ScriptBaseDir"] = getScriptBaseDir().string();
    body["Common"] = boost::json::object{
            {"LogFile",       getLogFile().string()},
#ifdef ENABLE_ALIMONITOR
            {"ScriptBaseDir", getScriptBaseDir().string()},
#endif
    };
    //TcpListener
    // body["TcpListener"]["Port"] = getInnerListenPort();
    // body["TcpListener"]["ClientPort"] = getHttpListenPort();
    // body["TcpListener"]["AppPort"] = getAppListenPort();
    // body["TcpListener"]["Ip"] = getLocalhost();
    body["TcpListener"] = boost::json::object{
            {"Ip",         getLocalhost()},
            {"Port",       getInnerListenPort()},
#ifdef ENABLE_ALIMONITOR
            {"ClientPort", getHttpListenPort()},
            {"AppPort",    getAppListenPort()},
#endif
    };
    //Optimize
    // body["Optimize"]["MaxMsgQueueSize"] = getMaxMsgQueueSize();
    // body["Optimize"]["MaxProcNum"] = getMaxProcNum();
    // body["Optimize"]["MaxOutputLen"] = getMaxOutputLen() / (1024 * 1024);
    // body["Optimize"]["MaxMemUse"] = getMaxMemUse() / (1024 * 1024);
    // body["Optimize"]["MaxRegisterIntv"] = getMaxRegisterIntv();
    // body["Optimize"]["ScheduleIntv"] = getScheduleIntv();
    // body["Optimize"]["ScheduleFactor"] = getScheduleFactor();
    body["Optimize"] = boost::json::object{
            {"MaxProcNum",      getMaxProcNum()},
            {"MaxOutputLen",    getMaxOutputLen() / (1024 * 1024)},
#ifdef ENABLE_ALIMONITOR
            {"MaxMsgQueueSize", getMaxMsgQueueSize()},
            {"MaxMemUse",       getMaxMemUse() / (1024 * 1024)},
            {"MaxRegisterIntv", getMaxRegisterIntv().count()},
#endif
            {"ScheduleIntv",    ToMicros(getScheduleIntv())},
            {"ScheduleFactor",  getScheduleFactor().count()},
    };
#ifdef ENABLE_ALIMONITOR
    //RCAddress
    // body["RCAddress"]["Port"] = getRcPort();
    body["RCAddress"] = boost::json::object{
            {"Port", getRcPort()},
    };
#endif

    // ostringstream sout;
    // sout << body << endl;
    // return sout.str();
    return boost::json::serialize(body);
}

string Config::GetValue(const string &key, const string &defaultValue) const {
    if (mPropertiesFile == nullptr) {
        return defaultValue;
    }
    return mPropertiesFile->GetValue(key, defaultValue);
}

static bool ReadMasterListConf(const string &file, std::vector<std::string> &masterHostVector) {
    masterHostVector = ReadFileLines(file);
    return !masterHostVector.empty();
    // char buf[128];
    // FILE *p_file = fopen(file.c_str(), "r");
    // if (p_file == nullptr) {
    //     return false;
    // }
    // while (!feof(p_file)) {
    //     memset(buf, 0, 128);
    //     if (fgets(buf, 128, p_file) == nullptr) {
    //         break;
    //     }
    //     string host = string(buf);
    //     if (host.size() > 0) {
    //         masterHostVector.push_back(host);
    //     }
    // }
    // fclose(p_file);
    // if (masterHostVector.empty()) {
    //     return false;
    // }
    // return true;
}

#ifdef ENABLE_ALIMONITOR
std::vector<std::string> & Config::getMasterHosts()
{
    string masterFile = GetValue("agent.master.conf.path","/home/staragent/conf/argusMaster.conf");
    vector<string> localMasterHostVector;
    if(ReadMasterListConf(masterFile,localMasterHostVector))
    {
        m_config.masterHostVector = localMasterHostVector;
    }
    return m_config.masterHostVector; 
}
int Config::getMasterPort()
{
	return GetValue("agent.master.port", m_config.masterPort);
}
#endif

bool Config::GetShennongOn() const
{
    return mShennongOn;
}

void Config::SetShennongOn(bool const &isShennongOn)
{
    mShennongOn = isShennongOn;
}

bool Config::GetIsTianjiEnv() const
{
    return mIsTianjiEnv;
}
void Config::SetIsTianjiEnv(bool const &isTianjiEnv)
{
    mIsTianjiEnv = isTianjiEnv;
}

int Config::getInnerListenPort() {
    return GetValue("agent.inner.listen.port", m_config.innerListenerPort);
}

#ifdef ENABLE_ALIMONITOR
std::set<std::string> Config::getLocalIps() {
    if (m_localIps.empty()) {
        m_localIps = SystemUtil::getLocalIp();
    }
    return m_localIps;
}
#endif