#include "TaskManager.h"

#include <unordered_set>

#include "common/Logger.h"
#include "common/SystemUtil.h"
#include "common/JsonValueEx.h"
#include "common/StringUtils.h"
#include "common/Chrono.h"
#include "base_parse_metric.h"

using namespace common;
using namespace std;

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
bool isTimeEffective(const std::string &cronExpr, const common::TimePeriod &timePeriod,
                     const std::chrono::system_clock::time_point &now) {
    return cronExpr.empty() || cronExpr == QUARTZ_ALL || timePeriod.in(now);
}
bool isNowEffective(const std::string &cronExpr, const common::TimePeriod &timePeriod) {
    using namespace std::chrono;
    return cronExpr.empty() || cronExpr == QUARTZ_ALL || timePeriod.in(system_clock::now());
}

namespace argus {
    tagCron &tagCron::operator=(const std::string &r) {
        if (this->expr != r) {
            this->expr = r;
            if (!empty()) {
                timePeriod.parse(this->expr.c_str());
            } else {
                timePeriod.reset();
            }
        }
        return *this;
    }

    bool tagCron::operator==(const tagCron &rhs) const {
        return expr == rhs.expr;
    }

    bool tagCron::empty() const {
        return expr.empty() || expr == QUARTZ_ALL;
    }

    bool PingItem::operator==(PingItem const &rhs) const {
        return taskId == rhs.taskId &&
               host == rhs.host &&
               effective == rhs.effective &&
               interval == rhs.interval;
    }

    bool TelnetItem::operator==(TelnetItem const &rhs) const {
        return taskId == rhs.taskId &&
               uri == rhs.uri &&
               requestBody == rhs.requestBody &&
               keyword == rhs.keyword &&
               effective == rhs.effective &&
               interval == rhs.interval &&
               negative == rhs.negative;
    }

    bool HttpItem::operator==(HttpItem const &rhs) const {
        return taskId == rhs.taskId &&
               uri == rhs.uri &&
               method == rhs.method &&
               requestBody == rhs.requestBody &&
               header == rhs.header &&
               keyword == rhs.keyword &&
               effective == rhs.effective &&
               interval == rhs.interval &&
               negative == rhs.negative &&
               timeout == rhs.timeout;
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    std::string NodeItem::str(bool withName) const {
        std::ostringstream oss;
        if (withName) {
            oss << "NodeItem";
        }

#define ROW(F) '"' << #F << "\":\"" << (F) << '"'
        oss << "{"
            << ROW(instanceId) << ","
            << ROW(serialNumber) << ","
            << ROW(aliUid) << ","
            << ROW(hostName) << ","
            << ROW(operatingSystem) << ","
            << ROW(region)
            << "}";
#undef ROW
        return oss.str();
    }


    bool parse(const std::string &taskName, const json::Object &obj, LabelAddInfo &data) {
        data.name = obj.getString("name");
        data.value = obj.getString("value");
        data.type = obj.getNumber("type", data.type);

        bool failed = data.type < 0 || data.name.empty();
        if (failed) {
            LogWarn("skip invalid {} add_labels item,type={},name={}", taskName, data.type, data.name);
        }
        return !failed;
    }

    bool parse(const std::string &taskName, const json::Array &arr, std::vector<LabelAddInfo> &data) {
        if (!arr.isNull()) {
            const size_t count = arr.size();
            data.reserve(data.size() + count);

            for (size_t i = 0; i < count; i++) {
                json::Object obj = arr[i].asObject();
                LabelAddInfo item;
                if (!obj.isNull() && parse(fmt::format("{}[{}]", taskName, i), obj, item)) {
                    data.push_back(item);
                }
            }
        }
        return !arr.isNull();
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LabelAddInfo 
    std::string LabelAddInfo::toString() const {
        return boost::json::serialize(toJson());
    }
    boost::json::object LabelAddInfo::toJson() const {
        return boost::json::object{
                {"name",  name},
                {"value", this->value},
                {"type",  type},
        };
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// tagKeyValues
    tagKeyValues::tagKeyValues(const std::string &k, const std::vector<std::string> &v) {
        this->key = k;
        this->endA = 0;
        if (!v.empty()) {
            const size_t count = v.size();
            this->values.resize(count);
            // 关于Pattern分类: https://aone.alibaba-inc.com/v2/project/1078985/bug/55962877
            for (size_t i = 0; i < count; i++) {
                const std::string &pattern = v[i];
                if (!pattern.empty() && pattern[0] == '!') {
                    // A类，所有OK才OK
                    this->values[endA++] = pattern.substr(1);
                } else {
                    // B类，任一OK即OK
                    this->values[count - 1 - (i - endA)] = pattern;
                }
            }
        }
    }

    tagKeyValues::tagKeyValues(const std::string &k): tagKeyValues(k, std::vector<std::string>{}) {
    }

    bool tagKeyValues::empty() const {
        return key.empty();
    }

    tagKeyValues tagKeyValues::fromJson(const json::Object &root) {
        std::string key = root.getString("label_name");
        std::vector<std::string> values;
        if (!key.empty()) {
            values = root.getArray("label_values").toStringArray();
        }
        return {key, values};
    }
    std::vector<tagKeyValues> tagKeyValues::fromJson(const json::Array &arr) {
        std::vector<tagKeyValues> ret;
        if (!arr.empty()) {
            ret.reserve(arr.size());
            arr.forEach([&](size_t index, const json::Object &obj) {
                tagKeyValues item{tagKeyValues::fromJson(obj)};
                if (!item.empty()) {
                    ret.push_back(item);
                }
            });
        }
        RETURN_RVALUE(ret);
    }

    struct tagWildcard {
        const bool head;
        const bool tail;

        tagWildcard(bool h, bool t) : head(h), tail(t) {}

        tagWildcard(const tagWildcard &r) : tagWildcard(r.head, r.tail) {}

        bool operator<(const tagWildcard &r) const {
            return head < r.head || (head == r.head && tail < r.tail);
        }
    };

    const auto &wildcardMatch = *new std::map<tagWildcard,std::function<bool(const std::string &, const std::string &)>>{
            {{true,  true},  [](const std::string &l, const std::string &r) { return StringUtils::Contain(l, r); }},
            {{true,  false}, [](const std::string &l, const std::string &r) { return StringUtils::EndWith(l, r); }},
            {{false, true},  [](const std::string &l, const std::string &r) { return StringUtils::StartWith(l, r); }},
            {{false, false}, [](const std::string &l, const std::string &r) { return l == r; }},
    };

    // Doc: https://aone.alibaba-inc.com/v2/project/1078985/bug/55962877
    bool tagKeyValues::IsMatch(const std::map<std::string, std::string> &data) const {
        auto it = data.find(this->key);
        if (it == data.end()) {
            return false;
        }

        auto doMatch = [](const std::string &pattern, const std::string &value) -> bool {
            std::string item = pattern;

            // 允许前后的通配
            tagWildcard wildcard(('*' == *item.cbegin()), ('*' == *item.crbegin()));
            if (wildcard.head || wildcard.tail) {
                size_t pos = wildcard.head ? 1 : 0;
                size_t end = item.size() - (wildcard.tail ? 1 : 0);
                item = item.substr(pos, (end > pos ? end - pos : 0));
            }

            return wildcardMatch.find(wildcard)->second(value, item);
        };

        const std::string &value = it->second;
        const size_t count = this->values.size();

        // (is_empty(A) || all(A)) && (is_empty(B) || any(B))
        // 优先处理A类
        for (size_t i = 0; i < count; i++) {
            if (doMatch(this->values[i], value)) {
                // i < this->endA ? false : true, 等同于下面判断
                return i >= this->endA;
            }
        }
        return this->endA == count; // B为空
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AliModuleItemData
    bool AliModuleItemData::parseJson(const json::Object &obj, const std::string &moduleName) {
        // CommonJson::GetJsonIntValue(item, "interval", aliItem.interval);
        interval = std::chrono::seconds{obj.getNumber("interval", ToSeconds(interval))};
        // CommonJson::GetJsonStringValue(item, "module_name", aliItem.moduleName);
        // aliItem.moduleName = item.getString("module_name");
        // CommonJson::GetJsonStringValue(item, "label_name", aliItem.tagName);
        if (IsZero(this->interval)) {
            LogWarn("skip invalid baseMetricItem, module_name={}, interval={}", moduleName, this->interval);
            return false;
        }
        // CommonJson::GetJsonStringValues(item, "metric_list", this->metrics);
        this->metrics = obj.getArray("metric_list").toStringArray();
        // CommonJson::GetJsonStringValues(item, "label_values", this->tagValues);
        // this->tagName = item.getString("label_name");
        // this->tagValues = item.getArray("label_values").toStringArray();
        this->whitelist = tagKeyValues::fromJson(obj.getArray("whitelist"));
        this->blacklist = tagKeyValues::fromJson(obj.getArray("blacklist"));
        tagKeyValues filterItem{tagKeyValues::fromJson(obj)}; // 兼容旧格式
        if (!filterItem.empty()) {
            this->whitelist.push_back(filterItem);
        }
        parse("baseMetric", obj.getArray("add_labels"), this->labelAddInfos);

        return true;
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 本地工具函数
    template<typename T>
    static bool safeGet(const SafeShared<T> &src, T &dst) {
        std::shared_ptr<T> data;
        bool ok = src.Get(data);
        if (ok) {
            dst = *data;
        }
        return ok;
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AliModuleItem
    bool AliModuleItem::parseJson(const json::Object &obj) {
        this->moduleName = obj.getString("module_name");
        if (this->moduleName.empty()) {
            LogWarn("skip invalid baseMetricItem: module_name is empty");
            return false;
        }
        return AliModuleItemData::parseJson(obj, this->moduleName);
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TaskManager 
    TaskManager::TaskManager() {
        // mModuleItems = std::make_shared<std::map<std::string, ModuleItem>>();
        // mScriptItems = std::make_shared<std::map<std::string, ScriptItem>>();
        // mAliScriptItems = std::make_shared<std::map<std::string, ScriptItem>>();
        // mReceiveItems = std::make_shared<std::map<int, ReceiveItem>>();
        // mProcessCollectItems = std::make_shared<vector<ProcessCollectItem>>();
        // mPingItems = std::make_shared<std::unordered_map<std::string, PingItem>>();
        // mTelnetItems = std::make_shared<std::unordered_map<std::string, TelnetItem>>();
        // mHttpItems = std::make_shared<std::unordered_map<std::string, HttpItem>>();
    }

    // std::shared_ptr<std::map<std::string, ModuleItem>> TaskManager::GetModuleItems() {
    //     return mModuleItemMap.Get();
    // }
    //
    // void TaskManager::GetModuleItems(std::map<std::string, ModuleItem> &s, const std::string &mid) {
    //     mModuleItemMap.Find(mid, s);
    // }
    //
    // void TaskManager::SetModuleItems(const std::shared_ptr<std::map<std::string, ModuleItem>> &r) {
    //     if (auto scheduler = SingletonArgusManager::Instance()->getModuleScheduler()) {
    //         scheduler->setModuleItems(r);
    //     }
    //     // mModuleItems = s;
    // }
    // std::shared_ptr<std::map<std::string, ModuleItem>> TaskManager::ModuleItems() const {
    //     if (auto scheduler = SingletonArgusManager::Instance()->getModuleScheduler()) {
    //         return scheduler->getModuleItems();
    //     }
    //     return nullptr;
    // }

    // int TaskManager::GetModuleItemsNum() {
    //     return mModuleItemMap.size();
    // }

    // void TaskManager::GetScriptItems(std::map<std::string, ScriptItem> &s, bool setChange) {
    //     mScriptItemMap.Get(s, setChange);
    // }
    //
    // void TaskManager::GetScriptItems(std::map<std::string, ScriptItem> &s, const std::string &mid) {
    //     mScriptItemMap.Find(mid, s);
    // }
    //
    // void TaskManager::SetScriptItems(const std::map<std::string, ScriptItem> &s) {
    //     mScriptItemMap.Set(s);
    // }
    //
    // int TaskManager::GetScriptItemsNum() {
    //     return mScriptItemMap.size();
    // }
    //
    // void TaskManager::getScriptItemKeys(std::set<std::string> &s) {
    //     mScriptItemMap.GetKeys(s, false);
    // }
    void TaskManager::SetScriptItems(const std::shared_ptr<std::map<std::string, ScriptItem>>& s) {
        mScriptItems = s;
    }

    // //aliScript
    // void TaskManager::GetAliScriptItems(std::map<std::string, ScriptItem> &s, bool setChange) {
    //     mAliScriptItemMap.Get(s, setChange);
    // }
    //
    // void TaskManager::GetAliScriptItems(std::map<std::string, ScriptItem> &s, const std::string &mid) {
    //     mAliScriptItemMap.Find(mid, s);
    // }
    //
    // void TaskManager::SetAliScriptItems(const std::map<std::string, ScriptItem> &s) {
    //     mAliScriptItemMap.Set(s);
    // }
    //
    // int TaskManager::GetAliScriptItemsNum() {
    //     return mAliScriptItemMap.size();
    // }
    //
    // void TaskManager::getAliScriptItemKeys(std::set<std::string> &s) {
    //     mAliScriptItemMap.GetKeys(s, false);
    // }
    void TaskManager::SetAliScriptItems(const std::shared_ptr<std::map<std::string, ScriptItem>> &s) {
        mAliScriptItems = s;
    }

    //receive
    // void TaskManager::GetReceiveItems(std::map<int, ReceiveItem> &s) {
    //     mReceiveItemMap.Get(s, true);
    // }
    //
    // void TaskManager::SetReceiveItems(const std::map<int, ReceiveItem> &s) {
    //     mReceiveItemMap = s;
    // }
    //
    // int TaskManager::GetRevieveItemsNum() {
    //     return mReceiveItemMap.size();
    // }
    void TaskManager::SetReceiveItems(const std::shared_ptr<std::map<int, ReceiveItem>> &s) {
        mReceiveItems = s;
    }

    //processCollectItems
    // void TaskManager::GetProcessCollectItems(vector<ProcessCollectItem> &s) {
    //     mProcessCollectItems.Get(s, true);
    // }
    //
    // void TaskManager::SetProcessCollectItems(const vector<ProcessCollectItem> &s) {
    //     mProcessCollectItems = s;
    // }
    //
    // int TaskManager::GetGetProcessCollectItemsNum() {
    //     return mProcessCollectItems.size();
    // }
    void TaskManager::SetProcessCollectItems(const std::shared_ptr<vector<ProcessCollectItem>> &s) {
        mProcessCollectItems = s;
    }

    //HpcCluterItem
    void TaskManager::SetHpcClusterItem(const HpcClusterItem &s) {
        mHpcClusterItem = s;
    }

    void TaskManager::GetHpcClusterItem(HpcClusterItem &s) {
        s = mHpcClusterItem.Get();
    }

    //PingItems
    // void TaskManager::GetPingItems(vector<PingItem> &s, bool setChange) {
    //     mPingItems.GetValues(s, setChange);
    // }
    //
    // void TaskManager::SetPingItems(std::unordered_map<std::string, PingItem> &s) {
    //     mPingItems.SetIfDifferent(s);
    //     // 对于没有改变的任务不需要重新加载
    // }
    //
    // int TaskManager::GetPingItemsNum() {
    //     return mPingItems.size();
    // }
    // void TaskManager::SetPingItemsChangedStatus(bool status) {
    //     if (status) {
    //         mPingItems = std::make_shared<std::unordered_map<std::string, PingItem>>(*mPingItems);
    //     }
    // }
    void TaskManager::SetPingItems(const std::shared_ptr<std::unordered_map<std::string, PingItem>> &s) {
        mPingItems = s;
    }
    //TelnetItems
    // void TaskManager::GetTelnetItems(vector<TelnetItem> &s, bool setChange) {
    //     mTelnetItems.GetValues(s, setChange);
    // }
    //
    // void TaskManager::SetTelnetItems(std::unordered_map<string, TelnetItem> const &s) {
    //     mTelnetItems.SetIfDifferent(s);
    //     // 对于没有改变的任务不需要重新加载
    // }
    //
    // int TaskManager::GetTelnetItemsNum() {
    //     return mTelnetItems.size();
    // }
    void TaskManager::SetTelnetItems(const std::shared_ptr<std::unordered_map<std::string, TelnetItem>> &s) {
        mTelnetItems = s;
    }

    //HttpItems
    // void TaskManager::GetHttpItems(vector<HttpItem> &s, bool setChange) {
    //     mHttpItems.GetValues(s, setChange);
    // }
    //
    // void TaskManager::SetHttpItems(std::unordered_map<std::string, HttpItem> const &s) {
    //     mHttpItems.SetIfDifferent(s);
    //     // 对于没有改变的任务不需要重新加载
    // }
    //
    // int TaskManager::GetHttpItemsNum() {
    //     return mHttpItems.size();
    // }
    void TaskManager::SetHttpItems(const std::shared_ptr<std::unordered_map<std::string, HttpItem>> &s) {
        mHttpItems = s;
    }

    void TaskManager::SetCmsDetectAddTags(const vector<LabelAddInfo> &labelAddInfos) {
        auto addTagMap = std::make_shared<map<string, string>>();
        BaseParseMetric::ParseAddLabelInfo(labelAddInfos, *addTagMap, nullptr);
        mCmsDetectAddTags = addTagMap;
    }

    void TaskManager::GetCmsDetectAddTags(map<string, string> &addTags) {
        addTags = *mCmsDetectAddTags.Get();
    }

    void TaskManager::SetCmsProcessAddTags(const vector<LabelAddInfo> &labelAddInfos) {
        auto newTags = std::make_shared<std::map<std::string, std::string>>();
        BaseParseMetric::ParseAddLabelInfo(labelAddInfos, *newTags, nullptr);
        mCmsProcessAddTags = newTags;
    }

    void TaskManager::GetCmsProcessAddTags(map<string, string> &addTags) {
        addTags = *mCmsProcessAddTags.Get();
    }

    void TaskManager::SetTopNItem(const TopNItem &item) {
        auto newItem = std::make_shared<TopNItem>(item);
        newItem->labelTags.clear();
        BaseParseMetric::ParseAddLabelInfo(item.labelAddInfos, newItem->labelTags, nullptr);

        mTopNItem = newItem;
    }

    void TaskManager::GetTopNItem(TopNItem &item) {
        std::shared_ptr<TopNItem> data;
        if (mTopNItem.Get(data)) {
            item = *data;
        }
    }

    //NodeItem
    void TaskManager::SetNodeItem(const NodeItem &s) {
        mNodeItem.Set(std::make_shared<NodeItem>(s));
    }

    void TaskManager::GetNodeItem(NodeItem &s) {
        safeGet(mNodeItem, s);
    }

    void TaskManager::SetCloudAgentInfo(const CloudAgentInfo &s) {
        mCloudAgentInfo = std::make_shared<CloudAgentInfo>(s);
    }

    void TaskManager::GetCloudAgentInfo(CloudAgentInfo &s) const {
        safeGet(mCloudAgentInfo, s);
    }

    //MetricItems
    // void TaskManager::GetMetricItems(vector<MetricItem> &s, bool ) {
    //     s = *mMetricItems.Get();
    // }
    //
    // void TaskManager::SetMetricItems(const vector<MetricItem> &s) {
    //     mMetricItems = MakeShared(s);
    // }
    //
    // int TaskManager::GetMetricItemsNum() {
    //     return mMetricItems->size();
    // }
    void TaskManager::SetMetricItems(const std::shared_ptr<vector<MetricItem>> &s) {
        mMetricItems = s;
    }


    //ExporterItems
    // void TaskManager::GetExporterItems(vector<ExporterItem> &s, bool setChange) {
    //     mExporterItems.Get(s, setChange);
    // }
    //
    // void TaskManager::GetShennongExporterItems(vector<ExporterItem> &s, bool setChange) {
    //     mShennongExporterItems.Get(s, setChange);
    // }
    //
    // void TaskManager::SetExporterItems(const vector<ExporterItem> &s) {
    //     mExporterItems = s;
    // }
    //
    // int TaskManager::GetExporterItemsNum() {
    //     return mExporterItems.size();
    // }
    // void TaskManager::SetExporterItems(const std::shared_ptr<vector<ExporterItem>> &s) {
    //     mExporterItems = s;
    // }

    // void TaskManager::SetShennongExporterItems(const vector<ExporterItem> &s) {
    //     mShennongExporterItems = s;
    // }
    //
    // int TaskManager::GetShennongExporterItemsNum() {
    //     return mShennongExporterItems.size();
    // }
    // void TaskManager::SetShennongExporterItems(const std::shared_ptr<vector<ExporterItem>> &s) {
    //     mShennongExporterItems = s;
    // }

    // void TaskManager::GetHttpReceiveItem(HttpReceiveItem &item, bool setChange) {
    //     mHttpReceiveItem.Get(item, setChange);
    // }
    //
    // void TaskManager::SetHttpReceiveItem(const HttpReceiveItem &item) {
    //     mHttpReceiveItem = item;
    // }
    void TaskManager::SetHttpReceiveItem(const std::shared_ptr<HttpReceiveItem> &s) {
        mHttpReceiveItem = s;
    }

    // void TaskManager::GetAliModuleItems(std::map<std::string, AliModuleItem> &s, bool setChange) {
    //     mAliModuleItemMap.Get(s, setChange);
    // }
    //
    // void TaskManager::SetAliModuleItems(const std::map<std::string, AliModuleItem> &s) {
    //     mAliModuleItemMap.Set(s);
    // }
    void TaskManager::SetAliModuleItems(const std::shared_ptr<std::map<std::string, AliModuleItem>> &s) {
        mAliModuleItems = s;
    }
}