#include "ali_metric.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/JsonValueEx.h"

using namespace std;
using namespace common;

namespace argus {
    AliMetric::AliMetric(const std::vector<MetricFilterInfo> &metricFilterInfos,
                         const std::vector<LabelAddInfo> &labelAddInfos)
            : BaseParseMetric(metricFilterInfos, labelAddInfos) {
        AliMetric::buildMap(metricFilterInfos, mMetricMap);
    }

    AliMetric::AliMetric(const std::vector<LabelAddInfo> &labelAddInfos)
            : AliMetric(std::vector<MetricFilterInfo>{}, labelAddInfos) {
    }

    void AliMetric::buildMap(const std::vector<MetricFilterInfo> &metricFilterInfos,
                             std::unordered_map<std::string, std::set<std::string>> &m) {
        for (const auto &it: metricFilterInfos) {
            size_t index = it.name.find('$');
            if (index == string::npos) {
                LogWarn("skip invalid metric:{} with no $", it.name);
                continue;
            }
            string type = it.name.substr(0, index);
            string metric = it.name.substr(index + 1);
            m[type].insert(metric);
        }
    }

    enum BaseParseMetric::ParseErrorType AliMetric::AddMetric(const string &metricStr) {
        //check content
        // JsonValue jsonValue;
        // if (jsonValue.parse(metricStr) == false)
        // {
        //     LogInfo("this alimetric result is not json format,skip!");
        //     LogDebug("{}",metricStr.c_str());
        //     return PARSE_FAILED;
        // }
        // Json::Value value = jsonValue.getValue();
        std::string error;
        json::Object value = json::parseObject(metricStr);
        if (value.isNull()) {
            LogWarn("this alimetric result is not valid json, skip! error: {}, json: {}", error, metricStr);
            return PARSE_FAILED;
        }

        bool status = value.getBool("success");
        // bool status = false;
        // CommonJson::GetJsonBoolValue(value,"success",status);
        if (!status) {
            LogInfo("skip invalid status this alimetric result!");
            return PARSE_FAILED;
        }
        // Json::Value dataValue;
        // if(!CommonJson::GetJsonObjectValue(value,"data",dataValue))
        // {
        //     LogInfo("skip invalid data-field this alimetric result!");
        //     return PARSE_FAILED;
        // }
        json::Object dataValue = value.getObject("data");
        if (dataValue.isNull()) {
            LogWarn("skip invalid data-field this alimetric result!");
            return PARSE_FAILED;
        }
        for (auto it = mMetricMap.begin(); it != mMetricMap.end(); it++) {
            // Json::Value arrayValue;
            // if(!CommonJson::GetJsonArrayValue(dataValue,it->first,arrayValue))
            // {
            //     LogInfo("no type({}) in this alimetric result!",it->first.c_str());
            //     continue;
            // }
            json::Array arrayValue = dataValue.getArray(it->first);
            if (arrayValue.isNull()) {
                LogInfo("no type({}) in this alimetric result!", it->first);
                continue;
            }
            // for (size_t i = 0; i < arrayValue.size(); i++) {
            arrayValue.forEach([&](size_t, const json::Object &item) {
                CommonMetric commonMetric;
                // Json::Value item = arrayValue.get(i, Json::Value::null);
                commonMetric.name = item.getString("metric");
                if (commonMetric.name.empty()) {
                    LogInfo("skip metric-item with invalid metric");
                    return;
                }
                if (it->second.find(commonMetric.name) == it->second.end()) {
                    // LogDebug("skip not-need metric:{}",metric.c_str());
                    return;
                }

                commonMetric.timestamp = item.getNumber<int64_t>("timestamp", -1);
                if (commonMetric.timestamp < 0) {
                    LogInfo("skip metric-item with invalid timestamp");
                    return;
                }

                int interval = item.getNumber<int>("interval");
                if (interval <= 0) {
                    LogInfo("skip metric-item with invalid interval");
                    return;
                }

                if (!item.getValue("value").isNumber()) {
                    LogInfo("skip metric-item with invalid value");
                    return;
                }
                commonMetric.value = item.getNumber<double>("value", 0.0);
                // if(!CommonJson::GetJsonDoubleValue(item,"value",value))
                // {
                //     LogInfo("skip metric-item with invalid value");
                //     continue;
                // }
                //对齐时间戳
                //保证一个周期只有一条数据
                commonMetric.timestamp = (commonMetric.timestamp / 1000) / interval * interval;
                if (commonMetric.timestamp < mLastMetricMap[commonMetric.name] + interval) {
                    LogDebug("skip invalid value with {}, {}:{}",
                             commonMetric.name, commonMetric.timestamp, mLastMetricMap[commonMetric.name]);
                    return;
                }

                mLastMetricMap[commonMetric.name] = commonMetric.timestamp;
                mCommonMetrics.push_back(commonMetric);
            });
        }
        return PARSE_SUCCESS;
    }
}