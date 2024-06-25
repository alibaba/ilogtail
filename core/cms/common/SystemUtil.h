#ifndef __SYSTEM_UTIL_
#define __SYSTEM_UTIL_

#include <string>
#include <set>
#include "Common.h"

namespace common {
#include "test_support"
class SystemUtil {

public:
    // 获取本地的主ip hostname -i 的结果
    static std::string getMainIp();
    // 公有云获取本机主IP
    static std::string getMainIpInCloud();
    // 获取所有本地的ip
#ifndef DISABLE_ALIMONITOR
    static std::set<std::string> getLocalIp();
#endif
    // 判断IP能否bind
    static bool IpCanBind(const std::string &ip);
    // 简单判断是不是公网的ip等非法情况
    static bool isInvalidIp(const std::string &ip, bool testPublic);
    // 
    static bool isPublicIp(const std::string &ip);

    static std::string getHostname();
    static std::string getDmideCode();
    static std::string getSn();
    static std::string parseDmideCode(const std::string &result);
    static std::string GetPathBase(std::string filePath);
    static std::string GetTianjiCluster();
    static std::string getRegionId(bool ignoreEnv = false);
#ifdef WIN32
    static std::string WindowsUTF8String(const char *src);
    static std::string getWindowsDmidecode(bool lower = false);
    static std::string GetWindowsDmidecode36();
    static std::string wmicPath(bool quote);
#endif
    static std::string GetEnv(const std::string &key);
    static void UnsetEnv(const std::string &key);
    static void SetEnv(const std::string &key, const std::string &value);
    static bool IsContainer();

    static int execCmd(const std::string &cmd, std::string *out = nullptr);
    static int execCmd(const std::string &cmd, std::string &out);

    static std::string ParseCluster(const std::string &clusterInfo);
    static bool IsContainerByInitProcMount();
    static bool IsContainerByPouchEnv();
    static bool IsContainerByInitProcSched();
};
#include "test_support"

}

#endif
