#ifndef ARGUS_CORE_TASK_MANAGER_H
#define ARGUS_CORE_TASK_MANAGER_H

#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <boost/json.hpp>

#include "common/Singleton.h"
#include "common/TimePeriod.h"
#include "common/SafeShared.h"

namespace json {
    class Object;
    class Array;
}

bool isTimeEffective(const std::string &cronExpr, const common::TimePeriod &,
                     const std::chrono::system_clock::time_point &);
bool isNowEffective(const std::string &cronExpr, const common::TimePeriod &timePeriod);
template<typename T>
bool isTimeEffective(const T &obj, const std::chrono::system_clock::time_point &now) {
    return isTimeEffective(obj.scheduleExpr, obj.timePeriod, now);
}
template<typename T>
bool isNowEffective(const T &obj) {
    return isNowEffective(obj.scheduleExpr, obj.timePeriod);
}

namespace argus
{

#define QUARTZ_ALL "[* * * * * ?]"

struct ModuleItem
{
    std::string mid;
    std::string scheduleExpr;
    std::string collectUrl;
    // int scheduleIntv = 0;//单位为ms
    std::chrono::milliseconds scheduleInterval{0};

    std::string name;
    std::string moduleArgs;
    std::string moduleVersion;
    std::string module;
    bool isSoType = true;
    common::TimePeriod timePeriod;
    std::vector<std::pair<std::string, std::string>> outputVector;

    bool isTimeEffective(const std::chrono::system_clock::time_point &now) const {
        return ::isTimeEffective(*this, now);
    }
    bool isNowEffective() const {
        return ::isNowEffective(*this);
    }
};

// struct ModuleItemEx: ModuleItem {
//     bool moduleInitOk = true;
// };

struct LabelAddInfo
{
    std::string name;
    std::string value;
    int type = -1; // 具体意义参见: BaseParseMetric::ParseAddLabelInfo

    bool operator<(const LabelAddInfo &x) const {
        return name > x.name;
    }

    std::string toString() const;
    boost::json::object toJson() const;
};
bool parse(const std::string &taskName, const json::Object &obj, LabelAddInfo &data);
bool parse(const std::string &taskName, const json::Array &arr, std::vector<LabelAddInfo> &);
inline bool parseAddLabels(const std::string &taskName, const json::Array &arr, std::vector<LabelAddInfo> &data) {
    return parse(taskName, arr, data);
}

struct MetricMeta{
    std::string name;
    // 1 for number, 2 for std::string
    int type = -1;
    bool isMetric = false;

    inline bool operator < (const MetricMeta &x) const{
        return name>x.name;
    }
};
//exporter info design
struct MetricFilterInfo
{
    std::string name;
    std::string metricName;
    std::map<std::string, std::string> tagMap;

    inline bool operator < (const MetricFilterInfo &x) const{
        return name>x.name;
    }
};
enum ScriptResultFormat{
    RAW_FORMAT,
    PROMETHEUS_FORMAT,
    ALIMONITOR_JSON_FORMAT,
    ALIMONITOR_TEXT_FORMAT,
    RAW_DATA_FORMAT,
};
struct ScriptItem
{
    std::string mid;
    std::string name;
    std::string collectUrl;
    std::string scheduleExpr;
    int protocolVersion = 1;
    std::chrono::seconds timeOut{20};
    std::chrono::seconds scheduleIntv{120};
    std::string scriptUser;
    std::vector<std::pair<std::string, std::string>> outputVector;

    std::chrono::seconds duration{0};
    std::chrono::system_clock::time_point firstSche;  // 设置的首次调度时间
    int reportStatus = 0;
    //是否为云监控阿里版脚本,后续统计可用于区分。
    bool isAliScript = false;
    enum ScriptResultFormat resultFormat = RAW_FORMAT;
    std::vector<LabelAddInfo> labelAddInfos;
    std::vector<MetricFilterInfo> metricFilterInfos;
    std::vector<MetricMeta> metricMetas;
    std::chrono::system_clock::time_point startTime;
    common::TimePeriod timePeriod;
};
struct ReceiveItem
{
    std::string name;
    int type = 0;
    std::vector<std::pair<std::string, std::string>> outputVector;
};

struct TagItem {
    std::string key;
    std::string value;

    TagItem() = default;

    TagItem(const std::string &k, const std::string &v) : key(k), value(v) {}
};

struct ProcessCollectItem
{
    std::string name;
    std::string processName;
    std::string processUser;
    // std::string command;
    std::vector<TagItem> tags;

    bool isEmpty() const {
        return name.empty() && processName.empty() && processUser.empty();
    }
};
struct HpcNodeInstance
{
    std::string instanceId;
    std::string ip;
};

struct HpcClusterItem {
    std::string clusterId;
    std::string regionId;
    std::string version;
    std::vector<HpcNodeInstance> hpcNodeInstances;
    bool isValid = false;
};

#include "common/test_support"
class tagCron {
private:
    std::string expr;
    common::TimePeriod timePeriod;

public:
    tagCron() = default;

    tagCron &operator=(const std::string &);
    bool empty() const;
    bool operator==(const tagCron &) const;
    const std::string &string() const {
        return expr;
    }
    bool in(const std::chrono::system_clock::time_point &now) const {
        return isTimeEffective(expr, timePeriod, now);
    }
};
#include "common/test_support"

// https://aone.alibaba-inc.com/v2/project/899492/req/55216495# 《【改进需求】可用性探测需要支持cron表达式》
struct PingItem {
    std::string taskId;
    std::string host;
    tagCron effective;
    std::chrono::seconds interval{60}; // 单位: 秒
    std::chrono::steady_clock::time_point lastScheduleTime;

    PingItem() = default;
    PingItem(std::string taskId, std::string host, const std::chrono::seconds &intv)
    {
        this->taskId = std::move(taskId);
        this->host = std::move(host);
        this->interval = intv;
    }

    bool operator==(PingItem const &rhs) const;

    bool operator!=(PingItem const &rhs) const {
        return !(rhs == *this);
    }
};

struct TelnetItem {
    std::string taskId;
    std::string uri;
    std::string requestBody;
    std::string keyword;
    tagCron effective;
    std::chrono::seconds interval{60};
    bool negative = false;
    std::chrono::steady_clock::time_point lastScheduleTime;

    TelnetItem() = default;

    TelnetItem(std::string taskId, std::string uri, const std::chrono::seconds &intv) {
        this->taskId = std::move(taskId);
        this->uri = std::move(uri);
        this->interval = intv;
    }

    bool operator==(TelnetItem const &rhs) const;

    bool operator!=(TelnetItem const &rhs) const {
        return !(rhs == *this);
    }
};

struct HttpItem {
    std::string taskId;
    std::string uri;
    std::string method;
    std::string requestBody;
    std::string header;
    std::string keyword;
    tagCron effective;
    std::chrono::seconds interval{60};
    std::chrono::seconds timeout{10};
    std::chrono::steady_clock::time_point lastScheduleTime;
    bool negative = false;

    HttpItem() = default;

    HttpItem(std::string taskId, std::string uri,
             const std::chrono::seconds &intv,
             const std::chrono::seconds &to = std::chrono::seconds{10},
             const std::string &method = {}) {
        this->taskId = std::move(taskId);
        this->uri = std::move(uri);
        this->interval = intv;
        this->timeout = to;
        this->method = method;
    }

    bool operator==(HttpItem const &rhs) const;

    bool operator!=(HttpItem const &rhs) const {
        return !(rhs == *this);
    }
};

struct NodeItem
{
    std::string instanceId;
    std::string serialNumber;
    std::string aliUid;
    std::string hostName;
    std::string operatingSystem;
    std::string region;

    std::string str(bool withName = false) const;
};
struct MetricItem
{
    std::string url = "https://cms-cloudmonitor.aliyun.com/agent/metrics/putLines";
    bool gzip = true;
    bool useProxy = true;
};
struct CloudAgentInfo
{
    std::string HeartbeatUrl;
    std::string proxyUrl;
    std::string user;
    std::string password;
    std::string accessKeyId;
    std::string accessSecret;
    std::string serialNumber;
};
struct ExporterItem
{
    std::string name;
    std::string target;
    std::string metricsPath;
    std::chrono::seconds interval{0};
    std::chrono::seconds timeout{0};
    std::vector<std::string> headers;
    int addStatus = 0;
    int type = 0;
    std::vector<MetricFilterInfo> metricFilterInfos;
    std::vector<LabelAddInfo> labelAddInfos;
    std::string mid;
    std::vector<std::pair<std::string, std::string>> outputVector;
};
struct HttpReceiveItem
{
    std::vector<MetricFilterInfo> metricFilterInfos;
    std::vector<LabelAddInfo> labelAddInfos;
    std::vector<std::pair<std::string, std::string>> outputVector;
};

// https://aone.alibaba-inc.com/v2/project/1078985/bug/55962877# 《【Argus】baseMetric.json配置支持黑白名单扩展逻辑》
class tagKeyValues {
    std::string key;
    size_t endA = std::string::npos;
    std::vector<std::string> values;

public:
    tagKeyValues() = default;
    explicit tagKeyValues(const std::string &);
    tagKeyValues(const std::string &, const std::vector<std::string> &);

    explicit tagKeyValues(const tagKeyValues &r) = default;
    explicit tagKeyValues(tagKeyValues &&r) = default;
    tagKeyValues &operator=(const tagKeyValues &r) = default;
    tagKeyValues &operator=(tagKeyValues &&) = default;

    std::string getKey() const {
        return key;
    }
    const std::vector<std::string> &getValues() const {
        return values;
    }
    bool empty() const;
    static tagKeyValues fromJson(const json::Object &);
    static std::vector<tagKeyValues> fromJson(const json::Array &);

    bool IsMatch(const std::map<std::string, std::string> &) const;

    template<typename L>
    static bool IsMatch(const L &l, const std::map<std::string, std::string> &data) {
        for (const auto &it: l) {
            if (it.IsMatch(data)) {
                return true;
            }
        }
        return false;
    }

    template<typename L1, typename L2>
    static bool IsDataValid(const L1 &blacklist, const L2 &whitelist,
                            const std::map<std::string, std::string> &data) {
        if (tagKeyValues::IsMatch(blacklist, data)) {
            return false;
        }
        if (tagKeyValues::IsMatch(whitelist, data)) {
            return true;
        }
        // 白名单为空则匹配，否则未匹配
        return whitelist.empty();
    }
};

struct AliModuleItemData {
    std::vector<tagKeyValues> blacklist; // https://aone.alibaba-inc.com/v2/project/1171635/bug/55843132
    std::vector<tagKeyValues> whitelist; // https://aone.alibaba-inc.com/v2/project/1171635/bug/55843132
    // std::string tagName;
    // std::vector<std::string> tagValues;
    std::vector<std::string> metrics;
    std::vector<LabelAddInfo> labelAddInfos;
    std::chrono::seconds interval{0};

    bool parseJson(const json::Object &, const std::string &);
};
struct AliModuleItem : AliModuleItemData {
    std::string moduleName;

    bool parseJson(const json::Object &);
};

struct TopNItem
{
    int topN = 0;
    std::vector<LabelAddInfo> labelAddInfos;
    std::map<std::string, std::string> labelTags;
};

    template<typename T>
    struct IDataStore {
        virtual ~IDataStore() = 0;
        virtual void set(const T &r) = 0;
        virtual T get() const = 0;
    };

    template<typename T>
    IDataStore<T>::~IDataStore() = default;

    template<typename T>
    class DefaultDataStore : public IDataStore<T> {
        T data;
    public:
        void set(const T &r) override {
            data = r;
        }
        T get() const override {
            return data;
        }
    };

    template<typename T, typename TMutex = std::mutex>
    class SafeDataStore {
        mutable TMutex mutex;
        std::shared_ptr<IDataStore<T>> data;
    public:
        SafeDataStore() {
            swap(nullptr);
        }
        SafeDataStore(const SafeDataStore &) = delete;
        SafeDataStore &operator=(const SafeDataStore &) = delete;

        std::shared_ptr<IDataStore<T>> swap(const std::shared_ptr<IDataStore<T>> &r) {
            std::unique_lock<std::mutex> lock(mutex);
            auto old = data;
            data = r;
            if (!data) {
                data = std::make_shared<DefaultDataStore<T>>();
            }
            return old;
        }

        void set(const T &r) {
            std::unique_lock<std::mutex> lock(mutex);
            data->set(r);
        }
        SafeDataStore &operator=(const T &r) {
            set(r);
            return *this;
        }

        T get() const {
            std::unique_lock<std::mutex> lock(mutex);
            return data->get();
        }
    };

class TaskManager
{
public:
    TaskManager();

    SafeDataStore<std::shared_ptr<std::map<std::string, ModuleItem>>> moduleItems;

    void SetModuleItems(const std::shared_ptr<std::map<std::string, ModuleItem>> &r) {
        moduleItems = r;
    }

    // // inline bool ModuleItemChanged() const
    // // {
    // //     return mModuleItemMap.IsChanged();
    // // }
    // int GetModuleItemsNum();
    std::shared_ptr<std::map<std::string, ModuleItem>> ModuleItems() const {
        return moduleItems.get();
    }

    //script
    // void GetScriptItems(std::map<std::string, ScriptItem> &s, bool setChange = false);
    // void GetScriptItems(std::map<std::string, ScriptItem> &s, const std::string &mid);
    // void SetScriptItems(const std::map<std::string, ScriptItem> &s);
    // int GetScriptItemsNum();
    // inline bool ScriptItemChanged() const
    // {
    //     return mScriptItemMap.IsChanged();
    // }
    // void getScriptItemKeys(std::set<std::string> &s);
    void SetScriptItems(const std::shared_ptr<std::map<std::string, ScriptItem>>&);
    const SafeSharedMap<std::string, ScriptItem> &ScriptItems() const {
        return mScriptItems;
    }

    //AliScript
    // void GetAliScriptItems(std::map<std::string, ScriptItem> &s, bool setChange = false);
    // void GetAliScriptItems(std::map<std::string, ScriptItem> &s, const std::string &mid);
    // void SetAliScriptItems(const std::map<std::string, ScriptItem> &s);
    // int GetAliScriptItemsNum();
    // inline bool AliScriptItemChanged() const
    // {
    //     return mAliScriptItemMap.IsChanged();
    // }
    // void getAliScriptItemKeys(std::set<std::string> &s);
    void SetAliScriptItems(const std::shared_ptr<std::map<std::string, ScriptItem>> &);
    const SafeSharedMap<std::string, ScriptItem> &AliScriptItems() const {
        return mAliScriptItems;
    }
    //recieve
    // void GetReceiveItems(std::map<int, ReceiveItem> &s);
    // void SetReceiveItems(const std::map<int, ReceiveItem> &s);
    // int GetRevieveItemsNum();
    // inline bool ReceiveItemChanged() const
    // {
    //     return mReceiveItemMap.IsChanged();
    // }
    void SetReceiveItems(const std::shared_ptr<std::map<int, ReceiveItem>> &);
    const SafeSharedMap<int, ReceiveItem> &ReceiveItems() const {
        return mReceiveItems;
    }

    //processCollectItems
    // void GetProcessCollectItems(std::vector<ProcessCollectItem> &s);
    // void SetProcessCollectItems(const std::vector<ProcessCollectItem> &s);
    // int GetGetProcessCollectItemsNum();
    // inline bool ProcessCollectItemsChanged() const
    // {
    //     return mProcessCollectItems.IsChanged();
    // }
    void SetProcessCollectItems(const std::shared_ptr<std::vector<ProcessCollectItem>> &);
    const SafeShared<std::vector<ProcessCollectItem>> &ProcessCollectItems() const {
        return mProcessCollectItems;
    }

    //HpcClusterItem
    void SetHpcClusterItem(const HpcClusterItem &s);
    void GetHpcClusterItem(HpcClusterItem &s);

    //CloudAgentInfo
    void SetCloudAgentInfo(const CloudAgentInfo &s);
    void GetCloudAgentInfo(CloudAgentInfo &s) const;

    //PingItems
    // void GetPingItems(std::vector<PingItem> &s, bool setChange = false);
    // void SetPingItems(std::unordered_map<std::string, PingItem> &s);
    // int GetPingItemsNum();
    // inline bool PingItemsChanged() const
    // {
    //     return mPingItems.IsChanged();
    // }
    // void SetPingItemsChangedStatus(bool status);
    // {
    //     mPingItems.SetChanged(status);
    //     // mPingItemChanged = status;
    // }
    void SetPingItems(const std::shared_ptr<std::unordered_map<std::string, PingItem>> &);
    const SafeShared<std::unordered_map<std::string, PingItem>> &PingItems() const {
        return mPingItems;
    }

    //TelnetItems
    // void GetTelnetItems(std::vector<TelnetItem> &s, bool setChange = false);
    // void SetTelnetItems(std::unordered_map<std::string, TelnetItem> const &s);
    // int GetTelnetItemsNum();
    // inline bool TelnetItemsChanged() const
    // {
    //     return mTelnetItems.IsChanged();
    // }
    // inline void SetTelnetItemsChangedStatus(bool status) 
    // {
    //     // mTelnetItemChanged = status;
    //     mTelnetItems.SetChanged(status);
    // }
    void SetTelnetItems(const std::shared_ptr<std::unordered_map<std::string, TelnetItem>> &);
    const SafeShared<std::unordered_map<std::string, TelnetItem>> &TelnetItems() const {
        return mTelnetItems;
    }

    //HttpItems
    // void GetHttpItems(std::vector<HttpItem> &s, bool setChange = false);
    // void SetHttpItems(std::unordered_map<std::string, HttpItem> const &s);
    // int GetHttpItemsNum();
    // inline bool HttpItemsChanged() const
    // {
    //     return mHttpItems.IsChanged();
    // }
    // inline void  SetHttpItemsChangedStatus(bool status)
    // {
    //     // mHttpItemChanged = status;
    //     mHttpItems.SetChanged(status);
    // }
    void SetHttpItems(const std::shared_ptr<std::unordered_map<std::string, HttpItem>> &);
    const SafeShared<std::unordered_map<std::string, HttpItem>> &HttpItems() const {
        return mHttpItems;
    }

    void SetCmsDetectAddTags(const std::vector<LabelAddInfo> &labelAddInfos);
    void GetCmsDetectAddTags(std::map<std::string, std::string> &addTags);

    void SetCmsProcessAddTags(const std::vector<LabelAddInfo> &labelAddInfos);
    void GetCmsProcessAddTags(std::map<std::string, std::string> &addTags);

    void SetTopNItem(const TopNItem &item);
    void GetTopNItem(TopNItem &item);

    //HpcClusterItem
    void SetNodeItem(const NodeItem &s);
    void GetNodeItem(NodeItem &s);

    //MetricItems
    // void GetMetricItems(std::vector<MetricItem> &s, bool setChange = false);
    // void SetMetricItems(const std::vector<MetricItem> &s);
    // int GetMetricItemsNum();
    // inline bool MetricItemsChanged() const
    // {
    //     return mMetricItems.IsChanged();
    // }
    void SetMetricItems(const std::shared_ptr<std::vector<MetricItem>> &);
    const SafeShared<std::vector<MetricItem>> &MetricItems() const {
        return mMetricItems;
    }
    // void GetExporterItems(std::vector<ExporterItem> &s, bool setChange = false);
    // void SetExporterItems(const std::vector<ExporterItem> &s);
    // int GetExporterItemsNum();
    // //ExporterItems
    // inline bool ExporterItemsChanged() const
    // {
    //     return mExporterItems.IsChanged();
    // }
    // void SetExporterItems(const std::shared_ptr<std::vector<ExporterItem>> &);
    // const SafeShared<std::vector<ExporterItem>> &ExporterItems() const {
    //     return mExporterItems;
    // }
    SafeDataStore<std::shared_ptr<std::vector<ExporterItem>>> exporterItems;
    SafeDataStore<std::shared_ptr<std::vector<ExporterItem>>> shennongExporterItems;

    void SetExporterItems(const std::shared_ptr<std::vector<ExporterItem>> &r) {
        exporterItems = r;
    }
    std::shared_ptr<std::vector<ExporterItem>> ExporterItems() const {
        return exporterItems.get();
    }
    void SetShennongExporterItems(const std::shared_ptr<std::vector<ExporterItem>> &r) {
        shennongExporterItems = r;
    }
    std::shared_ptr<std::vector<ExporterItem>> ShennongExporterItems() const {
        return shennongExporterItems.get();
    }

    // void GetShennongExporterItems(std::vector<ExporterItem> &s, bool setChange);
    // void SetShennongExporterItems(const std::vector<ExporterItem> &s);
    // int GetShennongExporterItemsNum();
    // inline bool ShennongExporterItemsChanged() const
    // {
    //     return mShennongExporterItems.IsChanged();
    // }
    // void SetShennongExporterItems(const std::shared_ptr<std::vector<ExporterItem>> &);
    // const SafeShared<std::vector<ExporterItem>> &ShennongExporterItems() const {
    //     return mShennongExporterItems;
    // }

    //ReceiveItems
    // void GetHttpReceiveItem(HttpReceiveItem &item, bool setChange = false);
    // void SetHttpReceiveItem(const HttpReceiveItem &item);
    // //ExporterItems
    // inline bool HttpReceiveItemChanged() const
    // {
    //     return mHttpReceiveItem.IsChanged();
    // }
    void SetHttpReceiveItem(const std::shared_ptr<HttpReceiveItem> &);
    const SafeShared<HttpReceiveItem> &GetHttpReceiveItem() const {
        return mHttpReceiveItem;
    }

    //AliModuleItem
    // void GetAliModuleItems(std::map<std::string, AliModuleItem> &s, bool setChange = false);
    // void SetAliModuleItems(const std::map<std::string, AliModuleItem> &s);
    // inline bool AliModuleItemChanged() const
    // {
    //     return mAliModuleItemMap.IsChanged();
    // }
    void SetAliModuleItems(const std::shared_ptr<std::map<std::string, AliModuleItem>> &);
    const SafeSharedMap<std::string, AliModuleItem> &AliModuleItems() const {
        return mAliModuleItems;
    }

    inline std::string GetAliModuleItemMd5() const
    {
        return mAlimoduleItemMd5;
    }
    inline void SetAliModuleItemMd5(const std::string &md5)
    {
        mAlimoduleItemMd5 = md5;
    }

private:
    //Module
    // std::map<std::string, ModuleItem> mModuleItemMap;
    // bool mModuleItemChanged = false;
    // InstanceLock mModuleLock;
    // SafeSharedMap<std::string, ModuleItem> mModuleItems;

    //Script
    // std::map<std::string, ScriptItem> mScriptItemMap;
    // bool mScriptItemChanged = false;
    // InstanceLock mScriptLock;
    SafeSharedMap<std::string, ScriptItem> mScriptItems;
    //AliScript
    // std::map<std::string, ScriptItem> mAliScriptItemMap;
    // bool mAliScriptItemChanged = false;
    // InstanceLock mAliScriptLock;
    SafeSharedMap<std::string, ScriptItem> mAliScriptItems;
    //Receive
    // std::map<int, ReceiveItem> mReceiveItemMap;
    // bool mReceiveItemChanged = false;
    // InstanceLock mReceiveLock;
    SafeSharedMap<int, ReceiveItem> mReceiveItems;
    //processCollectItem
    // std::vector<ProcessCollectItem> mProcessCollectItems;
    // bool mProcessCollectItemChanged = false;
    // InstanceLock mProcessCollectItemLock;
    SafeShared<std::vector<ProcessCollectItem>> mProcessCollectItems;
    //HpcClusterItem
    // HpcClusterItem mHpcClusterItem;
    // InstanceLock mHpcClusterItemLock;
    SafeT<HpcClusterItem> mHpcClusterItem;
    //CloudAgentInfo
    SafeShared<CloudAgentInfo> mCloudAgentInfo;
    //PingItem
    // std::unordered_map<std::string, PingItem> mPingItems;
    // std::atomic<bool> mPingItemChanged{false};
    // InstanceLock mPingItemLock;
    SafeShared<std::unordered_map<std::string, PingItem>> mPingItems;
    //TelnetItem
    // std::unordered_map<std::string, TelnetItem> mTelnetItems;
    // std::atomic<bool> mTelnetItemChanged{false};
    // InstanceLock mTelnetItemLock;
    SafeShared<std::unordered_map<std::string, TelnetItem>> mTelnetItems;
    //HttpItem
    // std::unordered_map<std::string, HttpItem> mHttpItems;
    // std::atomic<bool> mHttpItemChanged{false};
    // InstanceLock mHttpItemLock;
    SafeShared<std::unordered_map<std::string, HttpItem>> mHttpItems;

    //cmsDetect labelAddInfos
    // InstanceLock mCmsDetectAddTagsLock;
    // std::map<std::string, std::string> mCmsDetectAddTags;
    SafeSharedMap<std::string, std::string> mCmsDetectAddTags;

    //cmsProcess labelAddInfos
    // InstanceLock mCmsProcessAddTagsLock;
    // std::map<std::string, std::string> mCmsProcessAddTags;
    SafeSharedMap<std::string, std::string> mCmsProcessAddTags;

    //topNItem
    // InstanceLock mTopNItemLock;
    // TopNItem mTopNItem;
    SafeShared<TopNItem> mTopNItem;

    //NodeItem
    // NodeItem mNodeItem;
    // InstanceLock mNodeItemLock;
    SafeShared<NodeItem> mNodeItem;

    //MetricItem
    // std::vector<MetricItem> mMetricItems;
    // bool mMetricItemChanged{false};
    // InstanceLock mMetricItemLock;
    SafeShared<std::vector<MetricItem>> mMetricItems;

    //ExporterItem
    // std::vector<ExporterItem> mExporterItems;
    // bool mExporterItemChanged{false};
    // InstanceLock mExporterItemLock;
    // SafeShared<std::vector<ExporterItem>> mExporterItems;

    //ShennongExporterItem
    // std::vector<ExporterItem> mShennongExporterItems;
    // bool mShennongExporterItemChanged{false};
    // InstanceLock mShennongExporterItemLock;
    // SafeShared<std::vector<ExporterItem>> mShennongExporterItems;

    //HttpReceiveItem;
    // HttpReceiveItem mHttpReceiveItem;
    // bool mHttpReceiveItemChanged{false};
    // InstanceLock mHttpReceiveItemLock;
    SafeShared<HttpReceiveItem> mHttpReceiveItem;

    //AliModuleItem
    // std::map<std::string, AliModuleItem> mAliModuleItemMap;
    // bool mAliModuleItemChanged{false};
    // InstanceLock mAliModuleItemLock;
    SafeSharedMap<std::string, AliModuleItem> mAliModuleItems;
    //
    std::string mAlimoduleItemMd5;

};
typedef common::Singleton<TaskManager> SingletonTaskManager;
}

#endif
