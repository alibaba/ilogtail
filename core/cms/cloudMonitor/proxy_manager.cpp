#ifdef WIN32
#include <WinSock2.h> // depended by Ws2ipdef.h
#include <Ws2ipdef.h> // sockaddr_in6
#endif
#include "proxy_manager.h"

#include <thread>
#include <fmt/printf.h>

#include "common/Logger.h"
#include "common/NetWorker.h"
#include "common/StringUtils.h"
#include "common/HttpClient.h"
#include "common/PropertiesFile.h"
#include "common/CommonUtils.h"
#include "common/FileUtils.h"
#include "common/FilePathUtils.h" // GetExecPath, Which
#include "common/SystemUtil.h"
#ifdef ONE_AGENT
#include "cms/common/ThreadPool.h"
#else
#include "common/ThreadPool.h"
#endif
#include "common/ExecCmd.h"
#include "common/SyncQueue.h"
#include "common/FilePathUtils.h"
#include "common/Random.h"
#include "common/Config.h"

#include "core/TaskManager.h"

#include "sic/system_information_collector_util.h"

#ifdef min
#undef min
#endif

using namespace std;
using namespace common;
using namespace argus;

namespace cloudMonitor
{
int gCheckTimeout = 0;
static const struct {
    const char *regionId;
    const char *endpoint;
} BuiltinProxy[] = {
    {"cn-qingdao", "qdcmsproxy.aliyuncs.com:3128"},                    // 青岛
    {"cn-qingdao", "qdcmsproxy.aliyun.com:3128"},                      // 青岛
    {"cn-beijing", "bjcmsproxy.aliyuncs.com:3128"},                    // 北京
    {"cn-beijing", "bjcmsproxy.aliyun.com:3128"},                      // 北京
    {"cn-zhangjiakou", "cmsproxy-cn-zhangjiakou.aliyuncs.com:8080"},   // 张家口
    {"cn-zhangjiakou", "cmsproxy-cn-zhangjiakou.aliyun.com:8080"},     // 张家口
    {"cn-huhehaote", "cmsproxy-cn-huhehaote.aliyuncs.com:8080"},       // 呼和浩特
    {"cn-huhehaote", "cmsproxy-cn-huhehaote.aliyun.com:8080"},         // 呼和浩特
    {"cn-hangzhou", "hzcmsproxy.aliyuncs.com:3128"},                   // 杭州
    {"cn-hangzhou", "hzcmsproxy.aliyun.com:3128"},                     // 杭州
    {"cn-shanghai", "shcmsproxy.aliyuncs.com:3128"},                   // 上海
    {"cn-shanghai", "shcmsproxy.aliyun.com:3128"},                     // 上海
    {"cn-shenzhen", "szcmsproxy.aliyuncs.com:3128"},                   // 深圳
    {"cn-shenzhen", "szcmsproxy.aliyun.com:3128"},                     // 深圳
    {"cn-heyuan", "cmsproxy-cn-heyuan.aliyuncs.com:3128"},             // 河源
    {"cn-heyuan", "cmsproxy-cn-heyuan.aliyun.com:3128"},               // 河源
    {"cn-chengdu", "cmsproxy-cn-chengdu.aliyuncs.com:8080"},           // 成都
    // {"cn-chengju", "cmsproxy.cn-chengdu.aliyuncs.com"},
    {"cn-hongkong", "hkcmsproxy.aliyuncs.com:3128"},                   // 香港
    {"cn-hongkong", "hkcmsproxy.aliyun.com:3128"},                     // 香港
    {"us-west-1", "mgcmsproxy.aliyuncs.com:3128"},                     // 美西
    {"us-west-1", "mgcmsproxy.aliyun.com:3128"},                       // 美西

    {"us-east-1", "mgcmsproxy.aliyuncs.com:3128"},                     // 美东
    {"us-east-1", "mgcmsproxy.aliyun.com:3128"},                       // 美东
    {"ap-southeast-1", "xjpcmsproxy.aliyuncs.com:3128"},               // 新加坡
    {"ap-southeast-1", "xjpcmsproxy.aliyun.com:3128"},                 // 新加坡
    {"ap-southeast-2", "aucmsproxy.aliyuncs.com:8080"},                // 澳州
    {"ap-southeast-2", "aucmsproxy.aliyun.com:8080"},                  // 澳州
    {"ap-southeast-3", "cmsproxy-ap-southeast-3.aliyuncs.com:8080"},   // 吉隆坡
    {"ap-southeast-3", "cmsproxy-ap-southeast-3.aliyun.com:8080"},     // 吉隆坡
    {"ap-southeast-5", "cmsproxy-ap-southeast-5.aliyuncs.com:8080"},   // 雅加达
    {"ap-northeast-1", "jpcmsproxy.aliyuncs.com:8080"},                // 日本东京
    {"ap-northeast-1", "jpcmsproxy.aliyun.com:8080"},                  // 日本东京
    {"eu-central-1", "decmsproxy.aliyuncs.com:8080"},                  // 德国 法兰克福
    {"eu-central-1", "decmsproxy.aliyun.com:8080"},                    // 德国 法兰克福
    {"eu-west-1", "cmsproxy-eu-west-1.aliyuncs.com:8080"},             // 英国
    {"me-east-1", "dbcmsproxy.aliyuncs.com:8080"},                     // 迪拜
    {"me-east-1", "dbcmsproxy.aliyun.com:8080"},                       // 迪拜
    {"ap-south-1", "cmsproxy-ap-south-1.aliyuncs.com:8080"},           // 孟买
    {"private-domain@01", "opencmsproxy.aliyuncs.com:8080"},           // 专有域(private domain)
    {"private-domain@01", "opencmsproxy.aliyun.com:8080"},             // 专有域(private domain)
    {"private-domain@02", "vpc-opencmsproxy.aliyuncs.com:8080"},       // 专有域
    {"private-domain@02", "vpc-opencmsproxy.aliyun.com:8080"},         // 专有域
    {"private-domain@03", "hs.cms.aliyuncs.com:8080"},                 // 专有域
    {"private-domain@03", "hs.cms.aliyun.com:8080"},                   // 专有域

    // 2022-02-06 by chengjie.hcj
    {"cn-nantong",           "cmsproxy-cn-nantong.aliyuncs.com:3128"},
    {"cn-nanjing",           "cmsproxy-cn-nanjing.aliyun.com:3128"},
    {"cn-guangzhou",         "cmsproxy-cn-guangzhou.aliyun.com:3128"},
    // {"cn-huhehaote-nebula-1", "cmsproxy-cn-huhehaote-nebula-1.aliyun.com:3128"},
    {"cn-wulanchabu",        "cmsproxy-cn-wulanchabu.aliyun.com:3128"},
    // {"cn-zhangjiakou-spe", "cmsproxy-cn-zhangjiakou-spe.aliyun.com:3128"},
    {"cn-zhengzhou-jva",     "cmsproxy-cn-zhengzhou-jva.aliyuncs.com:3128"},
    {"cn-qingdao-nebula",    "cmsproxy-cn-qingdao-nebula.aliyuncs.com:3128"},
    {"rus-west-1",           "cmsproxy-rus-west-1.aliyun.com:3128"},
    {"ap-northeast-2",       "cmsproxy-ap-northeast-2.aliyuncs.com:3128"},
    {"cn-beijing-finance-1", "cmsproxy-cn-beijing-finance-1.aliyun.com:3128"}, // 北京金融云
    {"ap-hochiminh-ant",     "cmsproxy-ap-hochiminh-ant.aliyun.com:3128"},
    {"me-central-1",         "cmsproxy-me-central-1.aliyuncs.com:3128"},
    {"cn-shanghai-smt-huat", "cmsproxy-cn-shanghai-smt-huat.aliyuncs.com:3128"},
    {"cn-fuzhou",            "cmsproxy-cn-fuzhou.aliyuncs.com:3128"},
    {"cn-hangzhou-mybk",     "cmsproxy-cn-hangzhou-mybk.aliyun.com:3128"},
    {"ap-southeast-7",       "cmsproxy-ap-southeast-7.aliyun.com:3128"},
};
static constexpr size_t BuiltinProxyCount = sizeof(BuiltinProxy) / sizeof(BuiltinProxy[0]);

static void appendBuiltinProxy(std::list<ProxyInfo> &lstProxy, const std::string &user, const std::string &pwd) {
    auto append = [&](size_t i) {
        const auto &entry = BuiltinProxy[i % BuiltinProxyCount];

        ProxyInfo proxyInfo;
        proxyInfo.scheme = "http";
        proxyInfo.regionId = entry.regionId;
        proxyInfo.url = entry.endpoint;
        proxyInfo.user = user;
        proxyInfo.password = pwd;

        lstProxy.push_back(proxyInfo);
    };

    size_t start = Random<size_t>{BuiltinProxyCount - 1}(); // 闭区间, 随机访问
    const size_t end = start + BuiltinProxyCount;
    while (start < end) {
        append(start++);
    }
}


/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
std::string ProxyInfo::proxyUrl() const {
    if (url.empty()) {
        return {};
    } else if (scheme.empty()) {
        return url;
    } else {
        return scheme + "://" + url;
    }
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
void ProxyManager::waitDnsReady() const {
    // 2024-01-08.hancj 防止启动过快，导致dns尚未准备好
    auto detectDns = [](NetWorker &netWorker, const std::string &url) {
        std::vector<std::string> hosts = StringUtils::split(url, ':', {false, true});
        if (!hosts.empty() && netWorker.createAddr(hosts[0].c_str(), 80) != nullptr) {
            LogInfo("parse host {} ok,dns is ready!", hosts[0]);
            return true;
        }
        return false;
    };

    while (true) {
        NetWorker netWorker(__FUNCTION__);
        if (detectDns(netWorker, mHeartbeatHost)) {
            return;
        }
        if (!mIsAuto && !mProxyInfo.url.empty()) {
            if (detectDns(netWorker, mProxyInfo.url)) {
                return;
            }
        } else {
            // 手动配置的代理优先(当用户配置了代理时，如果再遍历内置proxy有可能会持续失败，从而导致agent极大增加了初始化时间)
            for (auto &proxyInfo: mProxyInfos) {
                if (detectDns(netWorker, proxyInfo.url)) {
                    return;
                }
            }
        }
        std::this_thread::sleep_for(5_s);
    }
}

static bool isHostPortOK(const std::string &host, const std::string &port, bool enableEmptyPort) {
    if (!host.empty()) {
        if (port.empty()) {
            return enableEmptyPort;
        }
        if (IsInt(port)) {
            // 合法的端口号
            int n = convert<int>(port);
            return 0 < n && n <= 65535;
        }
    }
    return false;
}

static std::string mustMatch(const std::string &key, const std::initializer_list<std::string> &allows) {
    if (!key.empty()) {
        for (const auto &item: allows) {
            if (item == key) {
                return key;
            }
        }
    }
    return *allows.begin();
}

// 不管成功与否，都会返回user/password
static bool loadHttpProxy(ProxyInfo &proxy, std::string &user, std::string &password) {
    auto *pConfig = SingletonConfig::Instance();

    user = TrimSpace(pConfig->GetValue("http.proxy.user", ""));
    password = TrimSpace(pConfig->GetValue("http.proxy.password", ""));
    if (user.empty() != password.empty()) {
        user.clear();
        password.clear();
    }

    string host = TrimSpace(pConfig->GetValue("http.proxy.host", ""));
    string port = TrimSpace(pConfig->GetValue("http.proxy.port", ""));
    if (isHostPortOK(host, port, true)) {
        proxy = ProxyInfo{}; // reset

        proxy.url = host;
        if (!port.empty()) {
            proxy.url += ":" + port;
        }

        auto scheme = TrimSpace(pConfig->GetValue("http.proxy.scheme", ""));
        proxy.scheme = mustMatch(scheme, {"http", "https", "https/2"});
        // 版本号
        auto pos = proxy.scheme.find('/');
        if (pos != std::string::npos) {
            proxy.schemeVersion = TrimSpace(proxy.scheme.substr(pos + 1));
            proxy.scheme = TrimSpace(proxy.scheme.substr(0, pos));
        }
        proxy.user = user;
        proxy.password = password;
    }

    return !proxy.url.empty();
}

// 只有成功才会更新roxy
static bool loadSocksProxy(ProxyInfo &proxy) {
    // See https://curl.se/libcurl/c/CURLOPT_PROXY.html
    // See https://blog.csdn.net/v6543210/article/details/123179755 # socks5和socks5h的区别
    auto *pConfig = SingletonConfig::Instance();

    std::string host = TrimSpace(pConfig->GetValue("socks5.proxy.host", ""));
    std::string port = TrimSpace(pConfig->GetValue("socks5.proxy.port", ""));
    // Unix domain sockets are supported for socks proxies since 7.84.0.
    // Set localhost for the host part. e.g. socks5h://localhost/path/to/socket.sock
    const char localhost[] = "localhost/";
    bool isUnixDomain = host.size() >= sizeof(localhost) && HasPrefix(host, localhost);
    if (isHostPortOK(host, port, isUnixDomain)) {
        proxy = ProxyInfo{};
        auto scheme = TrimSpace(pConfig->GetValue("socks5.proxy.scheme", ""));
        proxy.scheme = mustMatch(scheme, {"socks5h", "socks5", "socks4", "socks4a"});
        proxy.url = host;
        if (!isUnixDomain) {
            proxy.url += ":" + port;
        }
    }
    return !proxy.url.empty();
}

ProxyManager::ProxyManager()
{
    auto *pConfig = SingletonConfig::Instance();

    mIsAuto = pConfig->GetValue("http.proxy.auto", "true") == "true";

    mProxyInfos.clear();
    std::string proxyUser, proxyPassword;
    if (loadHttpProxy(mProxyInfo, proxyUser, proxyPassword)) {
        mProxyInfos.push_front(mProxyInfo);
    }
    appendBuiltinProxy(mProxyInfos, proxyUser, proxyPassword);
    if (loadSocksProxy(mProxyInfo)) {
        mProxyInfos.push_front(mProxyInfo);
    }

    // heartbeatUrl配置
    string heartBeatUrl = pConfig->GetValue("cms.agent.hosts", "https://cms-cloudmonitor.aliyun.com");
    heartBeatUrl = TrimSpace(heartBeatUrl);
    vector<string> heartBeatUrls = StringUtils::split(heartBeatUrl, ",", true);
    mHeartbeatUrl = heartBeatUrls[0];
    if (StringUtils::EndWith(mHeartbeatUrl, "/"))
    {
        mHeartbeatUrl = mHeartbeatUrl.substr(0, mHeartbeatUrl.size() - 1);
    }
    LogInfo("the heartbeatUrl is: {}", mHeartbeatUrl);
    //等待DNS就绪
    mHeartbeatHost = mHeartbeatUrl;
    size_t index = mHeartbeatHost.find("//");
    if (index != string::npos)
    {
        mHeartbeatHost = mHeartbeatHost.substr(index + 2);
    }
    index = mHeartbeatHost.find('/');
    if (index != string::npos)
    {
        mHeartbeatHost = mHeartbeatHost.substr(0, index);
    }
    LogInfo("the heartbeatHost is {}", mHeartbeatHost.c_str());

    waitDnsReady();
}

void ProxyManager::Init()
{
    gCheckTimeout += 2;
    mCheckTimeout += gCheckTimeout;
    if (mCheckTimeout > 10) {
        mCheckTimeout = 10;
    }
    ReadAk();
    mSerialNumber = GetSerialNumber();
    LogInfo("mSerialNumber={}, mSerialNumberFrom={}", mSerialNumber, mSerialNumberFrom);
    mProxyInfo = GetProxyInfo();

    CloudAgentInfo cloudAgentInfo;
    cloudAgentInfo.HeartbeatUrl = mHeartbeatUrl;
    cloudAgentInfo.proxyUrl = mProxyInfo.proxyUrl();
    cloudAgentInfo.user = mProxyInfo.user;
    cloudAgentInfo.password = mProxyInfo.password;
    cloudAgentInfo.accessKeyId = mAccessKeyId;
    cloudAgentInfo.accessSecret = mAccessSecret;
    cloudAgentInfo.serialNumber = mSerialNumber;
    LogInfo("proxyInfo: regionId={}, url={}, user={}, password={}",
            mProxyInfo.regionId, cloudAgentInfo.proxyUrl, mProxyInfo.user, mProxyInfo.password);
    SingletonTaskManager::Instance()->SetCloudAgentInfo(cloudAgentInfo);
}
void ProxyManager::ReadAk()
{
    defer(
            LogInfo("the accessKeyId is {}", mAccessKeyId);
            LogInfo("the accessSecret is {}", mAccessSecret);
    );

    auto *pConfig = SingletonConfig::Instance();
    mAccessKeyId = pConfig->GetValue("cms.agent.accesskey", "");
    mAccessSecret = pConfig->GetValue("cms.agent.secretkey", "");
    if (!mAccessKeyId.empty() && !mAccessSecret.empty()) {
        return;
    }

    auto baseDir = SingletonConfig::Instance()->getBaseDir();
    const char *const akFileName = "accesskey.properties";
    fs::path akFiles[] = {
            baseDir / "local_data" / "conf" / akFileName,
            baseDir / akFileName,
            GetExecDir() / akFileName,
    };
    for (auto &akFile: akFiles) {
        if (fs::exists(akFile)) {
            LogInfo("accesskey file: {}", akFile.string());
            unique_ptr<PropertiesFile> pPro(new PropertiesFile(akFile.string()));
            auto getAny = [&pPro](const std::initializer_list<std::string> &keys) {
                std::string v;
                for (auto it = keys.begin(); it != keys.end() && v.empty(); ++it) {
                    v = pPro->GetValue(*it, "");
                }
                return v;
            };
            mAccessKeyId = getAny({"cms.agent.accesskey", "CMS_AGENT_ACCESSKEY"});
            mAccessSecret = getAny({"cms.agent.secretkey", "CMS_AGENT_SECRETKEY"});
            if (!mAccessKeyId.empty() && !mAccessSecret.empty()) {
                break;
            }
        }
    }
}

static std::string GetEcsAssistMachineIdFile() {
#if defined(WIN32)
    return "C:\\ProgramData\\aliyun\\assist\\hybrid\\machine-id";
#else
    // 根据云助手同学反馈，不是托管实例的话，是没有这个文件的
    return "/usr/local/share/aliyun-assist/hybrid/machine-id";
#endif
}

// 从云助手获取序列号
// https://aliyuque.antfin.com/kongming/assistant/en4cqu1uc4trlw4i?singleDoc# 《云助手托管实例注册/注销同步到云监控》
std::string ProxyManager::GetSerialNumberFromEcsAssist(const std::string &machineIdFile) {
    std::string sn;
    if (fs::exists(machineIdFile)) {
        std::string errMsg;
        sn = ReadFileContent(machineIdFile, &errMsg);
        if (sn.empty() && !errMsg.empty()) {
            LogError("read sn from ecs-assist error: {}", errMsg.c_str());
        }
    }
    return sn;
}

std::string ProxyManager::GetSerialNumberFromEcsAssist() {
    return GetSerialNumberFromEcsAssist(GetEcsAssistMachineIdFile());
}

string ProxyManager::GetSerialNumber()
{
    std::vector<std::pair<std::string, std::function<std::string()>>> snSources;
    snSources.reserve(8);
    if (mAccessKeyId.empty()) {
        auto *cfg = SingletonConfig::Instance();
        // sn.skip.ecs.vpc.server=1时，跳过从100.200网段获取sn, 暨托管云助手优先
        if (!cfg->GetValue("sn.skip.ecs.vpc.server", false)) {
            snSources.emplace_back("VPCServer", GetSerialNumberFromVPC);
        }
        // 从云助手(ecs-assist)获取
        snSources.emplace_back("EcsAssist", static_cast<std::string(*)()>(GetSerialNumberFromEcsAssist)); // 重载选取
        // 从配置文件中看看
        snSources.emplace_back("Config File", [cfg]() { return cfg->GetValue("cms.agent.ecs.serialNumber", ""); });
        // 从OS获取
        snSources.emplace_back("OS", GetSerialNumberFromOS);
    }
    snSources.emplace_back("guid.New", GetSerialNumberFromLocal); // 生成

    std::string sn;
    for (const auto &pair: snSources) {
        sn = pair.second();
        if (!sn.empty()) {
            mSerialNumberFrom = pair.first;
            break;
        }
    }
    return sn;
}

string ProxyManager::GetSerialNumberFromVPC() {
    string vpcUrl = std::string(VPC_SERVER) + "/latest/meta-data/serial-number";
    return HttpGetString(vpcUrl, 5);
}

static string GetDmidecodeWithCmd(const string &cmd)
{
    std::string buf;
    if (getCmdOutput(cmd, buf) != 0) {
        return "";
    }
    // string result = string(buf);
    // StringUtils::trimSpaceCharacter(result);
    return SystemUtil::parseDmideCode(buf);
}

#ifdef WIN32
static string GetWin32SerialNumber()
{
    return SystemUtil::GetWindowsDmidecode36();
}
#elif defined(__APPLE__) || defined(__FreeBSD__)
static std::string GetMacOSSerialNumber() {
    // https://apple.stackexchange.com/questions/40243/how-can-i-find-the-serial-number-on-a-mac-programmatically-from-the-terminal
    std::string sn;
    ExecCmd(R"(system_profiler SPHardwareDataType | awk '/Serial/ {print $4}')", &sn);
    return sn;
}
#else
static string GetLinuxSerialNumber()
{
    fs::path binPath = Which("dmidecode", {SingletonConfig::Instance()->getBaseDir() / "bin"});
    LogInfo("dmidecode path {}!", binPath.string());
    if (binPath.empty()) {
        return "";
    }
    string cmd1 = binPath.string() + " -s system-serial-number";
    string dmidecode = GetDmidecodeWithCmd(cmd1);
    if (dmidecode.size() == 36) {
        return dmidecode;
    }
    cmd1 = binPath.string() + " -s system-uuid";
    return GetDmidecodeWithCmd(cmd1);
}
#endif

string ProxyManager::GetSerialNumberFromOS()
{
//利用dmidecode二进制读取
#ifdef WIN32
    return GetWin32SerialNumber();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    return GetMacOSSerialNumber();
#else
    return GetLinuxSerialNumber();
#endif
}

//利用dmidecode二进制读取
string ProxyManager::GetSerialNumberFromLocal()
{
    string serialNumber;
#ifdef WIN32
    char path[] = "SOFTWARE\\cloudmonitor";
    char keyName[] = "serial_number";
    char result[512] = {'\0'};
    HKEY hKey;
    DWORD dwDisposition;
    DWORD dwSize = 512;
    DWORD dwType;
    //打开或者创建
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
    {
        LogWarn("open register path {} error!",path);
        return "";
    }
    defer(RegCloseKey(hKey));

    int status = RegQueryValueEx(hKey, keyName, 0, &dwType, (LPBYTE)result, &dwSize);
    //值是否存在
    if (status == ERROR_SUCCESS)
    {
        serialNumber = string(result);
    }
    if (serialNumber.empty())
    {
        //值不存在
        serialNumber = CommonUtils::generateUuid();
        RegSetValueEx(hKey, keyName, 0, REG_SZ, (BYTE *)serialNumber.c_str(), static_cast<DWORD>(serialNumber.size()));
    }
#else
    auto *pConfig = SingletonConfig::Instance();
    string appDataDir = pConfig->GetValue("APPDATA", "/etc");
    string fileName = appDataDir + "/cloudmonitor/serial_number.properties";
    unique_ptr<PropertiesFile> pPro(new PropertiesFile(fileName));
    const string key = "cms.agent.serialNumber";
    serialNumber = pPro->GetValue(key, "");
    if (serialNumber.empty()) {
        serialNumber = CommonUtils::generateUuid();
        string fileContent = key + "=" + serialNumber;
        FileUtils::WriteFileContent(fileName, fileContent);
    }
#endif
    return serialNumber;
}

ProxyInfo ProxyManager::DetectFromProxyInfos() const {
#if 1 // !defined(WIN32)
    // 优势: 平均变快了
    // 劣势: 即便成功获取了regionId，也要等最差的那个跑完(差不多需要2秒)。
    //从配置的映射表中逐个尝试
    {
        SyncQueue<ProxyInfo> queue(mProxyInfos.size());
        const size_t maxThreadCount = std::min(size_t(100), mProxyInfos.size());
        auto proxyDetect = ThreadPool::Option{}.name("ProxyDetect").max(maxThreadCount).makePool();
        for (const auto &curProxy: mProxyInfos) {
            proxyDetect->commit({}, [&, curProxy]() {
                ProxyInfo proxyInfo;
                if (CheckProxyWithSerialNumber(curProxy, proxyInfo)) {
                    queue << proxyInfo;
                }
            });
        }
        proxyDetect->stop(); // 探测任务已全部提交，接下来等待执行完成即可。
        // 哨兵，保证所有的代理都探测完时队列关闭，以确保queue.Take一定会有返回
        std::thread guard{[queue, proxyDetect]() mutable {
            proxyDetect->join();
            queue.Close();
        }};
        defer(guard.join());

        ProxyInfo proxyInfo;
        if (queue.Take(proxyInfo)) {
            LogDebug("proxyInfo{{regionId: {}}}", proxyInfo.regionId);
            return proxyInfo;
        }
    }
#else
    // windows下并发的curl有问题，要么挂掉，要么狂吃cpu
    for (const auto &curProxy: mProxyInfos) {
        ProxyInfo proxyInfo;
        if (CheckProxyWithSerialNumber(curProxy, proxyInfo)) {
            return proxyInfo;
        }
    }
#endif
    ProxyInfo noProxyInfo;
    return noProxyInfo;
}

ProxyInfo ProxyManager::GetProxyInfo() const {
    if (!mIsAuto && !mProxyInfo.url.empty()) {
        // 如果手动设置代理则将手动设置的代理返回
        // 否则执行自动获取代理步骤
        return mProxyInfo;
    }
    //自动获取步骤
    string regionId = GetRegionIdFromVPC();
    if (!regionId.empty()) {
        //则从配置的映射表中找(根据regionId)
        ProxyInfo proxyInfo;
        if (GetProxyInfoWithRegionId(regionId, proxyInfo)) {
            return proxyInfo;
        }
    }

    return DetectFromProxyInfos();
}

string ProxyManager::GetRegionIdFromVPC() {
    return SystemUtil::getRegionId(true);
}

bool ProxyManager::GetProxyInfoWithRegionId(const string &regionId, ProxyInfo &proxyInfo) const
{
    bool haveConfigProxy = false;
    for (const auto &proxy: mProxyInfos) {
        if (proxy.regionId == regionId) {
            haveConfigProxy = true;
            for (int j = 0; j < 3; j++) {
                if (CheckHealth(proxy)) {
                    proxyInfo = proxy;
                    return true;
                }
            }
        }
    }
    //没有配置any tunnel，可能为新增加的region，尝试下自己拼凑；
    if (!haveConfigProxy) {
        const char *domains[] = {
                "aliyuncs.com:3128",
                "aliyun.com:3128",
        };
        for (const char *domain: domains) {
            ProxyInfo tmpProxyInfo;
            tmpProxyInfo.regionId = regionId;
            tmpProxyInfo.url = fmt::format("cmsproxy-{}.{}", regionId, domain);
            if (CheckHealth(tmpProxyInfo)) {
                proxyInfo = tmpProxyInfo;
                return true;
            }
        }
    }

    return false;
}
bool ProxyManager::CheckHealth(const ProxyInfo &proxyInfo) const
{
    string url = mHeartbeatUrl + "/check_health";
    int timeout = mCheckTimeout;
    bool status;
    return HttpGetStringWithProxy(url, proxyInfo, timeout, status) == "ok";
}

bool ProxyManager::CheckProxyWithSerialNumber(const ProxyInfo &proxyInfo, ProxyInfo &targetProxy) const
{
    auto *pConfig = SingletonConfig::Instance();
    string uri = pConfig->GetValue("cms.instanceId.uri", "/agent/latest/meta-data/region-id/%s");
    // char bufUrl[128];
    // apr_snprintf(bufUrl, 128, uri.c_str(), mSerialNumber.c_str());
    std::string bufUrl = fmt::sprintf(uri, mSerialNumber);
    string url = mHeartbeatUrl + string(bufUrl);
    bool status;
    string regionId = HttpGetStringWithProxy(url, proxyInfo, mCheckTimeout, status);
    if (!status)
    {
        return false;
    }
    //去掉引号
    if (StringUtils::StartWith(regionId, "\"") && StringUtils::EndWith(regionId, "\"") && regionId.size() > 2)
    {
        regionId = regionId.substr(1, regionId.size() - 2);
    }
    //正常情况，能过通过代理获取到regionId,该regionId应该和配置的代理的regionId一致
    targetProxy = proxyInfo;
    if (!regionId.empty() && regionId != "unknown" && regionId != proxyInfo.regionId)
    {
        // 香港经典网络下，使用cn-hongkong不通，反倒cn-qingdao的代理是通的
        // 对cn-hongkong进行测试;
        ProxyInfo regionIdProxyInfo;
        if (GetProxyInfoWithRegionId(regionId, regionIdProxyInfo))
        {
            targetProxy = regionIdProxyInfo;
            return true;
        }

        ProxyInfo proxyInfos[] = {
                proxyInfo,    // 指定代理, 继续尝试已经找到的proxy
                ProxyInfo{},  // 公网，无代理
        };
        for (auto &proxy: proxyInfos) {
            if (CheckHealth(proxy)) {
                targetProxy = proxy;
                return true;
            }
        }
        // //测试公网
        // ProxyInfo noProxyInfo;
        // if (CheckHealth(noProxyInfo))
        // {
        //     targetProxy = noProxyInfo;
        //     return true;
        // }
        return false;
    }
    return true;
}

string ProxyManager::HttpGetString(const string &url, int timeout)
{
    HttpRequest httpRequest;
    httpRequest.url = url;
    httpRequest.timeout = timeout;
    HttpResponse httpResponse;
    std::string result;
    if (HttpClient::HttpCurl(httpRequest, httpResponse) != HttpClient::Success) {
        LogInfo("try to connect ({})  error,errorInfo: {}", httpRequest.url, httpResponse.errorMsg);
    } else if (httpResponse.resCode != (int) HttpStatus::OK) {
        LogInfo("try to connect ({})  error,response code is:{}", httpRequest.url, httpResponse.resCode);
    } else {
        LogInfo("try to connect ({}) ok,response msg is:{}", httpRequest.url, httpResponse.result);
        result = httpResponse.result;
    }
    return result;
}

string ProxyManager::HttpGetStringWithProxy(const string &url, const ProxyInfo &proxyInfo, int timeout, bool &status) {
#if defined(WIN32) || defined(ENABLE_COVERAGE)
    // Windows下几乎都是超时
    // 单测环境下，由于对阿里云各region都是开放的，因此也几乎都是超时，但线上则不然。
    const int maxTry = 1;
#else
    const int maxTry = 3;  // 网络超时的情况下重试三次
#endif
    for (int i = 0; i < maxTry; i++) {
        HttpRequest httpRequest;
        httpRequest.url = url;
        httpRequest.timeout = timeout;
        if (!proxyInfo.url.empty()) {
            httpRequest.proxy = proxyInfo.proxyUrl();
            httpRequest.proxySchemeVersion = proxyInfo.schemeVersion;
            httpRequest.user = proxyInfo.user;
            httpRequest.password = proxyInfo.password;
        }
        HttpResponse httpResponse;
        int code = HttpClient::HttpCurl(httpRequest, httpResponse);
        if (code != HttpClient::Success) {
            LogInfo("try to connect ({}) with proxy({}) error, curlCode: {}(IsTimeout: {}, {}s)), errorInfo: {}",
                    httpRequest.url, httpRequest.proxy, code, HttpClient::IsTimeout(code), timeout, httpResponse.errorMsg);
            status = false;
            if (HttpClient::IsTimeout(code)) {
                continue;
            }
            return "";
        } else if (httpResponse.resCode != (int) HttpStatus::OK) {
            LogInfo("try to connect ({}) with proxy({}) error, response code is: {}",
                    httpRequest.url, httpRequest.proxy, httpResponse.resCode);
            status = false;
            return "";
        } else {
            status = true;
            LogInfo("try to connect ({}) with proxy({}) ok, response msg is: {}",
                    httpRequest.url, httpRequest.proxy, httpResponse.result);
            return httpResponse.result;
        }
    }

    return "";
}

} // namespace cloudMonitor
