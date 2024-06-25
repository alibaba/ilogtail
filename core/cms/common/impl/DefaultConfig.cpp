#include "common/DefaultConfig.h"

using namespace common;

ConfigProfile::ConfigProfile() {
    setDefault();
}

void ConfigProfile::setDefault() {
#ifdef ENABLE_ALIMONITOR
    //默认全局的master地址
    masterHostVector = std::vector<std::string>{
            "cdn.master.alibaba-inc.com",
            "cross.master.alibaba-inc.com",
            "inner.master.alibaba-inc.com",
            "cross.master.vip.tbsite.net",
            "100.67.148.212",
            "100.67.149.143",
            "11.132.11.114",
            "11.226.9.192",
            "106.11.223.145",
            "106.11.80.133",
    };
    //AppClient 监听，jmx信息的接入
    appListenerPort = 19777;
    //http脚本执行接口
    httpScriptRunListenerPort = 15778;
    //脚本下载使用到的端口
    rcPort = 19797;
    rcBackupPort = 80;
    //一次发送到monitor最大数据量
    maxMemUse = 128 * 1024 * 1024;
    //master定时器中使用，最大的间隔
    maxRegisterIntv = std::chrono::seconds{180};
    //Optimize
    //消息队列的最大长度
    maxMsgQueueSize = 4000;
    masterPort = 19666;

    //ClientListener，http数据接入
    httpListenerPort = 15776;
#endif

    //公用配置
    //进程最大数目（脚本执行时，避免资源消耗过大）
    maxProcNum = 100;
    //agent默认调动间隔，单位为us
    scheduleIntv = std::chrono::microseconds{500 * 1000};
    //agent默认的调度因子
    scheduleFactor = std::chrono::seconds{120};
    //执行脚本的最大输出
    maxOutputLen = 5 * 1024 * 1024;

    //以下为Argus的相关配置
    //环境变量
    envPath = "/bin:/usr/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/alibaba/java/bin:/opt/taobao/java/bin";
    //模块调度间隔，单位为us
    // mModuleScheduleIntv = std::chrono::microseconds{100 * 1000};
    //模块的最大输出
    // maxModuleOutputLen=1024*512;
#ifdef ENABLE_CLOUD_MONITOR
    moduleCollectThreadNum = 2;
    maxModuleCollectThreadNum = 5;
#else
    moduleCollectThreadNum = 5;
    maxModuleCollectThreadNum = 50;
#endif
    //内部监听InnerListener，windows使用tcp，linux使用domain-socket
    localhost = "127.0.0.1";
    innerListenerPort = 15579;
    mInnerSocketPath = "local_data/argus.sock";
    //外部数据接入
    mReceiveDomainSocketPath = "/tmp/argus.sock";
    mReceivePort = 15774;
}

DefaultConfig::DefaultConfig() = default;

ConfigProfile &DefaultConfig::getConfigProfile(const std::string &profile) {
    if (profile.find("super") != std::string::npos) {
        return m_super;
    } else if (profile.find("test") != std::string::npos) {
        return m_test;
    } else if (!profile.empty()) {
        return m_user;
    } else {
        return m_default;
    }
}
