#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif
#include "common/SystemUtil.h"
#include "common/Common.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/CommonUtils.h"
#include "common/ScopeGuard.h"
#include "common/HttpClient.h"
#include "common/NetWorker.h"
#include "common/JsonValueEx.h"
#include "common/NetTool.h"
#include "common/ChronoLiteral.h"

#include <fstream>
#include <boost/process/detail/traits/wchar_t.hpp> // fix error: 'is_wchar_t' is not a class template
#include <boost/process/env.hpp>

#include <apr-1/apr_network_io.h>

#ifdef WIN32
#include <Windows.h>
#include "sic/win32_system_information_collector.h"
#endif

// #ifdef LINUX
//
// #include <sys/types.h>
// #include <ifaddrs.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
//
// #endif

#include "common/FileUtils.h"
#include "common/SafeBase.h"
#include "common/ExecCmd.h"
#ifdef ENABLE_ALIMONITOR
#include "sic/system_information_collector.h" // SicInterfaceConfig
#endif

using namespace common;
using namespace std;

#ifdef ENABLE_ALIMONITOR
std::set<std::string> SystemUtil::getLocalIp() {
    std::vector<SicInterfaceConfig> ifConfigs;
    int ret = SystemInformationCollector::New()->SicGetInterfaceConfigList(ifConfigs);

    std::set<std::string> ipSet;
    if (ret == SIC_EXECUTABLE_SUCCESS) {
        for (const auto &it: ifConfigs) {
            std::string ipv4 = it.address.str();
            if (!ipv4.empty() && !isInvalidIp(ipv4, true)) {
                ipSet.insert(ipv4);
            }
            std::string ipv6 = it.address6.str();
            if (!ipv6.empty() && !isInvalidIp(ipv6, true)) {
                ipSet.insert(ipv6);
            }
        }
    }
    return ipSet;

    // #ifdef WIN32
    //     // windows need refer to the ** msdn
    //     std::set<std::string> ret;
    //     ret.insert("127.0.0.1");
    //     return ret;
    // #endif
    // #if defined(LINUX) || defined(__APPLE__) || defined(__FreeBSD__)
    //     std::set<std::string> ret;
    //     struct ifaddrs *ifAddrStruct = nullptr;
    //
    //     if (0 != getifaddrs(&ifAddrStruct))
    //     {
    //         return ret;
    //     }
    //     for (struct ifaddrs *ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
    //     {
    //         if (!ifa->ifa_addr)
    //         {
    //             continue;
    //         }
    //         std::string tmpIp;
    //         if (ifa->ifa_addr->sa_family == AF_INET)
    //         { // IPV4
    //             void *tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
    //             char addressBuffer[INET_ADDRSTRLEN];
    //             inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
    //             tmpIp = addressBuffer;
    //         } else if (ifa->ifa_addr->sa_family == AF_INET6) { // IPV6 unsupport
    //             void *tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
    //             char addressBuffer[INET6_ADDRSTRLEN];
    //             inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
    //             tmpIp = addressBuffer;
    //         }
    //         if (isInvalidIp(tmpIp))
    //         {
    //             continue;
    //         }
    //         ret.insert(tmpIp);
    //     }
    //     freeifaddrs(ifAddrStruct);
    //     return ret;
    // #endif
}
#endif

// 公网ip等非法ip的情况
bool SystemUtil::isInvalidIp(const std::string &ip, bool testPublic) {
    if (!ip.empty()) {
        if (testPublic && isPublicIp(ip)) {
            LogInfo("check if valid intranet ip failed, ip({}), drop this one", ip);
        } else {
            return !IpCanBind(ip);
        }
    }
    return true;
}

bool SystemUtil::isPublicIp(const std::string &ip) {
    bool failed = false;
    bool isPrivate = IP::IsPrivate(ip, &failed); // HasPrefixAny(ip, {"10.", "11.", "100.", "172.", "192.",});
    return !failed && !isPrivate;
}

int SystemUtil::execCmd(const std::string &cmd, std::string *out) {
    LogDebug("{}", cmd);
    return ExecCmd(cmd, out);
}

int SystemUtil::execCmd(const std::string &cmd, std::string &out) {
    return execCmd(cmd, &out);
}

// 获取主控ip的策略，序号越小优先级越高
#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
#   define MAIN_IP_USE_UDP
#else
static const char *mainIpCommands[] = {
        "hostname -i 2>/dev/null", // 有可能会返回多个IP
        // 2>/dev/null 屏蔽『Device "bond0" does not exist.』
        "ip ad show bond0 2>/dev/null | grep inet  | grep -v inet6 | awk -F ' ' '{print $2}' | awk -F '/' '{print $1}'",
        "ip ad show lo    2>/dev/null | grep lo:nc | grep inet | grep -v inet6 | awk -F ' ' '{print $2}' | awk -F '/' '{print $1}'",
        "ip ad show eth0  2>/dev/null | grep inet  | grep -v inet6 | awk -F ' ' '{print $2}' | awk -F '/' '{print $1}'",
};
#endif
std::string getMainIp(bool useLast, bool testPublicIp) {
#if defined(MAIN_IP_USE_UDP)
    string mainIp;
    NetWorker net(__FUNCTION__);
    if (0 == net.connect<UDP>("172.22.24.82", 16666)) {
        mainIp = net.getLocalAddr();
    }
    return mainIp;
#else
    std::string ip;
    for (const char *&cmd: mainIpCommands) {
        std::string ret;
        SystemUtil::execCmd(cmd, ret);
        ret = TrimSpace(ret);
        if (!ret.empty()) {
            for (const std::string &curIP: StringUtils::split(ret, ' ', true)) {
                ip = curIP;
                if (!SystemUtil::isInvalidIp(curIP, testPublicIp)) {
                    LogInfo("{}(useLast: {}, testPublicIp: {}) successfully, method[{}], ip={}, command=({})",
                            __FUNCTION__, useLast, testPublicIp, (&cmd - mainIpCommands), curIP, cmd);
                    return curIP;
                }
            }
        }
    }
    if (!useLast) {
        ip.clear();
    }
    LogInfo("{}(useLast: {}, testPublicIp: {}) , use <{}>", __FUNCTION__, useLast, testPublicIp, ip);
    return ip;
#endif
}

std::string SystemUtil::getMainIp() {
    return ::getMainIp(false, false);
}

std::string SystemUtil::getHostname() {
    std::string name = CommonUtils::getHostName();
    if (name.empty()) {
        return {"error hostname"};
    }
#ifdef WIN32
    name = WindowsUTF8String(name.c_str());
#endif
    return name;
//    char buf[1024];
//    if(gethostname(buf,1024)!=0){
//        return {"error hostname"};
//    }else{
//#ifdef WIN32
//        return SystemUtil::WindowsUTF8String(buf);
//#else
//        return {buf};
//#endif
//    }
}

#if defined(__APPLE__) || defined(__FreeBSD__)
static std::string getMacSerialNumber() {
    //    |   "IOPlatformSerialNumber" = "C02FQ1MCMD6R"
    std::string sn;
    SystemUtil::execCmd("ioreg -l | grep IOPlatformSerialNumber | awk '{print $4}'", sn);
    return Trim(sn, "\"");
}
#endif

std::string SystemUtil::getDmideCode() {
#ifdef WIN32
    return getWindowsDmidecode();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    return getMacSerialNumber();
#else
    const char *parameters[] = {
            " -s system-serial-number",
            " | awk  'BEGIN {FS=\":\"} /Serial Number/ {print $2; exit}'",
    };
    constexpr const size_t paramCount = sizeof(parameters) / sizeof(parameters[0]);

    std::string dmidecodeString;
    auto check = [&]() { return !dmidecodeString.empty() && dmidecodeString.size() <= 64; };

    fs::path path = Which("dmidecode");
    for (size_t i = 0; i < paramCount && !path.empty() && !check(); i++) {
        std::string result;
        execCmd(path.string() + parameters[i], result);
        result = TrimSpace(result);
        dmidecodeString = parseDmideCode(result);
    }

    LogDebug("dmidecode is {}", dmidecodeString);
    return dmidecodeString;
#endif

}

std::string SystemUtil::parseDmideCode(const std::string &result) {
    auto resultVector = StringUtils::splitString(result, "\n");

    std::string target = (resultVector.size() == 1 ? resultVector.at(0) : "");
    if (target.empty()) {
        unsigned int count = 0;
        for (const auto &str: resultVector) {
            if (!str.empty() && str[0] != '#') {
                ++count;
                if (!HasPrefix(str, "i-")) {
                    target = str;
                    break;
                } else {
                    // 多于一个"i-"前缀的数据，则忽略
                    target = (1 == count ? str : "");
                }
            }
        }
    }
    return target;
}

#ifdef WIN32
std::string SystemUtil::wmicPath(bool quote) {
    std::string wmic = (System32() / "wbem" / "wmic.exe").string();
    if (quote) {
        wmic = "\"" + wmic + "\"";
    }
    return wmic;
}

std::string SystemUtil::getWindowsDmidecode(bool lower)
{
    std::string cmd = fmt::format("{} bios get serialnumber /value < nul", wmicPath(true));
    std::string sn;
    execCmd(cmd, sn);
    string::size_type pos = sn.find('=');
    if (pos != string::npos) {
        sn.erase(0, pos + 1);
    }
    sn = TrimSpace(sn);
    if (sn.empty()) {
        LogInfo("execCmd({}) => {}", cmd, sn);
    }
    if (lower) {
        sn = StringUtils::ToLower(sn);
    }

    return sn;
}

// wmic csproduct list full:
// Description=计算机系统产品
// IdentifyingNumber=68ad59b9-f6d7-40e9-8b1c-93458f77b6b7   <-- 这个才是getWindowsDmidecode的输出
// Name=Alibaba Cloud ECS
// SKUNumber=
// UUID=68AD59B9-F6D7-40E9-8B1C-93458F77B6B7
// Vendor=Alibaba Cloud
// Version=pc-i440fx-2.1
std::string SystemUtil::GetWindowsDmidecode36()
{
    std::string dmidecode = getWindowsDmidecode(true);
    std::string result;
    if(dmidecode.size()!=36)
    {
        std::string cmd = fmt::format("{} csproduct list full", wmicPath(true));
        execCmd(cmd, result);
		std::vector<std::string> lines = StringUtils::split(result, "\n", true);
        for(const auto & line : lines)
        {
            string::size_type pos1 = line.find("UUID"); // Note: 此处与getWindowsDmidecode获取的并不相同。
            string::size_type pos2 = line.find("=");
            if (pos1 != string::npos && pos2 != string::npos) {
                string uuid = line.substr(pos2+1);
                // uuid.erase(0, pos2 + 1);
                LogInfo("execCmd({}) => {}", cmd, uuid);
                dmidecode = StringUtils::ToLower(TrimSpace(uuid));
                break;
            }
        }
    }
    return dmidecode;
}
#endif

static auto &globalSn = *new SafeT<std::string>();

std::string SystemUtil::getSn() {
    std::string sn;
    Sync(globalSn) {
        if (globalSn.empty()) {
#ifdef WIN32
            globalSn = getWindowsDmidecode();
#else
            std::string tmp = ReadFileContent("/usr/sbin/staragent_sn");
            // execCmd("cat /usr/sbin/staragent_sn",sn);
            if (tmp.empty()) {
                tmp = getDmideCode();
            }
            globalSn = TrimSpace(tmp);
#endif
        }
        sn = globalSn;
    }}}
    return sn;
}

std::string SystemUtil::GetPathBase(std::string filePath) {
    if (filePath.empty()) {
        return ".";
    }

//#ifdef WIN32
//    char pathSplit='\\';
//#else
//    char pathSplit = '/';
//#endif
//    if (filePath[filePath.size() - 1] == pathSplit) {
//        filePath = filePath.substr(0, filePath.size() - 1);
//    }
//    size_t index = filePath.find_last_of(pathSplit);
//    if (index != string::npos && index < filePath.size() - 1) {
//        return filePath.substr(index + 1);
//    } else {
//        return {&pathSplit, &pathSplit + 1};
//    }

    if (filePath.back() == '\\' || filePath.back() == '/') {
        filePath.pop_back();
    }
    if (!filePath.empty()) {
        fs::path path{filePath};
        if (path.has_filename()) {
            return path.filename().string();
        }
    }

    const char sep = fs::path::separator;
    return {&sep, &sep + 1};
}

std::string SystemUtil::GetTianjiCluster() {
    // const char *cmdFormat = "curl -s 'http://localhost:7070/api/v5/local/GetMachineInfoPackage?hostname=%s'";
    // string hostname = getHostname();
    // char cmdBuf[512];
    // apr_snprintf(cmdBuf,512,cmdFormat.c_str(),hostname.c_str());
    // string tianjiJson;
    // getCmdOutput(cmd, tianjiJson);
    // return ParseCluster(tianjiJson);

    const char *query = "http://localhost:7070/api/v5/local/GetMachineInfoPackage?hostname=";
    const std::string cmd = query + UrlEncode(getHostname());

    std::string result;
    HttpResponse response;
    int code = HttpClient::HttpGet(cmd, response);
    if (HttpClient::Success == code && response.resCode == 200) {
        result = ParseCluster(response.result);
    } else {
        LogError("GET {} => CurlCode: {}({}), HttpCode: {}, result: {}",
                 cmd, code, response.errorMsg, response.resCode, response.result);
    }
    return result;
}

std::string SystemUtil::ParseCluster(const std::string &tianjiJson) {
    std::string error;
    json::Object value = json::parseObject(tianjiJson, error);
    if (value.isNull()) {
        LogWarn("tianjiJson is invalid: {}, json: {}", error, tianjiJson);
        return std::string{};
    }
    auto result = value.getString({"data", "cluster"});
    if (!result.error.empty()) {
        LogWarn("tianjiJson invalid: {}, json: {}", result.error, tianjiJson);
        return std::string{};
    }

    LogDebug("the tianji cluster is: {}", result.result);

    return result.result;
}

#ifdef WIN32
std::string SystemUtil::WindowsUTF8String(const char *src)
{
    int len = MultiByteToWideChar(CP_ACP, 0, src, -1, nullptr, 0);
    unique_ptr<wchar_t[]>wstr(new wchar_t[len + 1]);
    unique_ptr<char[]>dest(new char[len * 4 + 1]);
    memset(wstr.get(), 0, len + 1);
    memset(dest.get(), 0, len * 2 + 1);
    MultiByteToWideChar(CP_ACP, 0, src, -1, wstr.get(), len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr.get(), -1, nullptr, 0, nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, wstr.get(), -1, (char*)dest.get(), len, nullptr, nullptr);
    return std::string(dest.get());
}
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4018)
#endif
std::string SystemUtil::GetEnv(const std::string &key) {
    auto env = boost::this_process::environment();
    auto it = env.find(key);
    std::string value;
    if (it != env.end()) {
        value = it->to_string();
    }
    return value;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

void SystemUtil::SetEnv(const std::string &key, const std::string &value) {
    if (!key.empty() && !value.empty()) {
        auto env = boost::this_process::environment();
        env[key] = value;
    }
}

void SystemUtil::UnsetEnv(const std::string &key) {
    if (!key.empty()) {
        auto env = boost::this_process::environment();
        env.erase(key);
    }
}

bool SystemUtil::IsContainerByInitProcSched() {
#ifdef WIN32
    return false;
#else
    //cat /proc/1/sched | head -1
    //docker: systemd (13155, #threads: 1)
    //not docker: systemd (1, #threads: 1)
    //内核版本4.19及以后该方法失效
    std::ifstream fileStream;
    defer(fileStream.close());

    std::string line;
    fileStream.open("/proc/1/sched", std::fstream::in);
    if (fileStream.fail()) {
        LogError("open file /proc/1/sched error : {}", strerror(errno));
    } else {
        std::getline(fileStream, line);
    }

    // (1,  --- 非容器
    return !line.empty() && !StringUtils::Contain(line, "(1,");
    // if (line.empty()) {
    //     return false;
    // }
    //
    // if (line.find("(1,") != std::string::npos) {
    //     //非容器
    //     return false;
    // }
    // return true;
#endif
}

bool SystemUtil::IsContainerByPouchEnv() {
#ifdef WIN32
    return false;
#endif
    //包含pouch_container_id和pouch_container_image环境变量为容器
    std::ifstream fileStream;
    ResourceGuard fileStreamGuard([&fileStream] {
        fileStream.close();
    });

    std::string line;
    fileStream.open("/proc/1/environ", std::fstream::in);
    if (fileStream.fail()) {
        LogError("open file /proc/1/environ error : {}", strerror(errno));
    } else {
        std::getline(fileStream, line);
    }

    return StringUtils::ContainAll(line, {"pouch_container_id", "pouch_container_image"});
}

bool SystemUtil::IsContainerByInitProcMount() {
#ifdef WIN32
    return false;
#endif
    //cat /proc/1/mounts | awk '$2=="/" && $3=="overlay"' | wc -l
    //alidocker/pouch只支持overlay这一种镜像格式，通过根分区的挂载方法可以判断出来是在容器中还是物理机上
    //1 是容器
    //0 是物理机
    //如果pouch支持了其他镜像格式，这个判断方法也失效
    const char *cmd = R"(cat /proc/1/mounts | awk '$2=="/" && $3=="overlay"' | wc -l)";

    std::string buffer;
    if (getCmdOutput(cmd, buffer) != 0) {
        LogError("command [{}] error :{}", cmd, buffer);
        return false;
    }

    return 1 == convert<int>(buffer);
}

bool SystemUtil::IsContainer() {
    return IsContainerByInitProcSched() || IsContainerByPouchEnv() || IsContainerByInitProcMount();
}

// 获取主控ip的策略，序号越小优先级越高
// 1. hostname -i
// 2. ip ad show bond0 | grep inet | awk -F ' ' '{print $2}' | awk -F '/' '{print $1}'
// 3. ip ad show lo | grep lo:nc | grep inet | awk -F ' ' '{print $2}' | awk -F '/' '{print $1}'
// 4. ip ad show eth0 | grep inet | awk -F ' ' '{print $2}' | awk -F '/' '{print $1}'
std::string SystemUtil::getMainIpInCloud() {
    return ::getMainIp(true, true);
}

bool SystemUtil::IpCanBind(const std::string &ip) {
    NetWorker netWorker(__FUNCTION__);
    return 0 == netWorker.bind(ip, 0);
    // apr_sockaddr_t *sockaddr = nullptr;
    // if (netWorker.createAddr(&sockaddr, ip.c_str(), 0) != 0) {
    //     LogInfo("create local addr failed for ip({}), drop this one", ip.c_str());
    //     return false;
    // }
    // if (netWorker.socket() != 0 || netWorker.setSockOpt(APR_SO_REUSEADDR, 1) != 0) {
    //     LogInfo("set socket options failed for ip({}), drop this one", ip.c_str());
    //     return false;
    // }
    // if (netWorker.bind(sockaddr, nullptr) != 0) {
    //     LogInfo("bind to local options failed for ip({}), drop this one", ip.c_str());
    //     return false;
    // }
    //
    // return true;
}

std::string SystemUtil::getRegionId(bool ignoreEnv) {
    if (!ignoreEnv) {
        string regionId = GetEnv("REGION_ID");
        if (!regionId.empty()) {
            return regionId;
        }
    }
    return HttpClient::GetString(std::string(VPC_SERVER) + "/latest/meta-data/region-id", 5_s);
}
