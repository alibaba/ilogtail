#ifndef ARGUS_CLOUD_MONITOR_PROXY_CONFIG_H
#define ARGUS_CLOUD_MONITOR_PROXY_CONFIG_H

#include <string>
#include <list>

namespace cloudMonitor {
    struct ProxyInfo {
        std::string scheme;         // socks5, socks5h, http, ...
        std::string schemeVersion;  // 仅支持https/2, 此时为2
        std::string regionId;
        std::string url;
        std::string password;
        std::string user;

        std::string proxyUrl() const;
    };

#include "common/test_support"
class ProxyManager {
public:
    ProxyManager();
    ~ProxyManager() = default;

    void Init();

    void ReadAk();

public:
    std::string GetSerialNumber();
    static std::string GetRegionIdFromVPC();
    static std::string GetSerialNumberFromVPC();
    static std::string GetSerialNumberFromOS();
    static std::string GetSerialNumberFromLocal();
    static std::string GetSerialNumberFromEcsAssist(const std::string &);
    static std::string GetSerialNumberFromEcsAssist();

private:
    ProxyInfo GetProxyInfo() const;
    bool GetProxyInfoWithRegionId(const std::string &regionId, ProxyInfo &proxyInfo) const;
    static std::string HttpGetString(const std::string &url, int timeout = 5);
    static std::string HttpGetStringWithProxy(const std::string &url, const ProxyInfo &proxyInfo, int timeout, bool &status);

    bool CheckProxyWithSerialNumber(const ProxyInfo &proxyInfo, ProxyInfo &targetProxy) const;
    bool CheckHealth(const ProxyInfo &proxyInfo) const;

    ProxyInfo DetectFromProxyInfos() const;
    void waitDnsReady() const;
private:
    std::list<ProxyInfo> mProxyInfos;
    ProxyInfo mProxyInfo;
    bool mIsAuto;
    // bool mUserSocks5Proxy;
    std::string mHeartbeatUrl;
    std::string mHeartbeatHost;
    std::string mAccessKeyId;
    std::string mAccessSecret;
    std::string mSerialNumber;
    std::string mSerialNumberFrom;
    int mCheckTimeout = 2;
};
#include "common/test_support"

}
#endif 
