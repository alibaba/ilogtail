#include "base_parse_metric.h"
#include "common/SystemUtil.h"

using namespace std;
using namespace common;

namespace argus
{
const int BaseParseMetric::ParseErrorTypeNum=PARSE_ERROR_TYPE_END;
const string BaseParseMetric::ParseErrorString[PARSE_ERROR_TYPE_END]={
    "success",
    "prometheus invalid line",
    "alimonitorJson result not json",
    "alimonitorJson result not object",
    "alimonitorJson MSG is not object or array",
    "alimonitorJson MSG array element invalid",
    "alimonitorJson MSG object member invalid",
    "alimonitorText parse timestmap failed",
    "alimonitorText parse key value pair failed",
    "script result is empty",
    "failed"
};

BaseParseMetric::BaseParseMetric(const vector<MetricFilterInfo>& metricFilterInfos,const vector<LabelAddInfo> &labelAddInfos)
{
    for (auto it = metricFilterInfos.begin(); it != metricFilterInfos.end(); it++) {
        mMetricFilterInfoMap[it->name] = *it;
    }
    BaseParseMetric::ParseAddLabelInfo(labelAddInfos, mAddTagMap, &mRenameTagMap);
}
int BaseParseMetric::GetCommonMetrics(vector<CommonMetric> &commonMetrics)
{
    for(const auto &it : mCommonMetrics)
    {
        common::CommonMetric metric = it;
        metric.tagMap.insert(mAddTagMap.begin(), mAddTagMap.end());
        // for(auto tagIt=mAddTagMap.begin();tagIt!=mAddTagMap.end();tagIt++)
        // {
        //     metric.tagMap[tagIt->first]=tagIt->second;
        // }
        for(auto & renameTagIt : mRenameTagMap)
        {
            auto replaceIt = metric.tagMap.find(renameTagIt.first);
            if(replaceIt != metric.tagMap.end() &&!renameTagIt.second.empty())
            {
                string value = replaceIt->second;
                metric.tagMap.erase(replaceIt);
                metric.tagMap[renameTagIt.second] = value;
            }
        }
        commonMetrics.push_back(metric);
    }
    mCommonMetrics.clear();
    return static_cast<int>(commonMetrics.size());
}

static const auto &keyProc = *new std::unordered_map<string, string(*)()>{
        {"ip",           SystemUtil::getMainIp},
        {"hostname",     SystemUtil::getHostname},
        {"__hostname__", SystemUtil::getHostname},
        {"sn",           SystemUtil::getSn},
        {"cluster",      SystemUtil::GetTianjiCluster},
        {"__cluster__",  SystemUtil::GetTianjiCluster},
};

std::string LabelGetter::get(const std::string &key) const {
    const auto targetIt = keyProc.find(key);
    return targetIt != keyProc.end()? targetIt->second(): std::string{};
}

bool BaseParseMetric::ParseAddLabelInfo(const std::vector<LabelAddInfo> &labelAddInfos,
                                        std::map<std::string, std::string> &addTagMap,
                                        std::map<std::string, std::string> *renameTagMap,
                                        const LabelGetter *labelGetter) {
    LabelGetter defaultGetter;
    if (labelGetter == nullptr) {
        labelGetter = &defaultGetter;
    }
    bool ret = true;
    for (const auto &it: labelAddInfos) {
        if (it.type == 0) {
            std::string val = labelGetter->get(it.name);
            addTagMap[it.name] = val;
            ret = ret && !val.empty();
        } else if (it.type == 1) {
            string tagValue = SystemUtil::GetEnv(it.value);
            if (!tagValue.empty()) {
                addTagMap[it.name] = tagValue;
            }
        } else if (it.type == 2) {
            addTagMap[it.name] = it.value;
        } else if (it.type == 3 && renameTagMap != nullptr) {
            (*renameTagMap)[it.name] = it.value;
        }
    }
    return ret;
}

// static
int BaseParseMetric::GetCommonMetricName(const CommonMetric &commonMetric,
                                         const std::map<std::string, MetricFilterInfo> &metricFilterInfoMap,
                                         string &metricName)
{
    if (!metricFilterInfoMap.empty()) {
        const auto it = metricFilterInfoMap.find(commonMetric.name);
        if (it == metricFilterInfoMap.end() || commonMetric.tagMap.size() < it->second.tagMap.size()) {
            return -1;
        }
        const MetricFilterInfo &metricFilterInfo = it->second;
        for (const auto &tag: metricFilterInfo.tagMap) {
            auto itM = commonMetric.tagMap.find(tag.first);
            if (itM == commonMetric.tagMap.end() || itM->second != tag.second) {
                return -1;
            }
        }

        metricName = it->second.metricName;
    }
    // metricFilterInfoMap为空时，返回0，但metricName为空，这逻辑挺诡异的。
    return 0;
}

}
