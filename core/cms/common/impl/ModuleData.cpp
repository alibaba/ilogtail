#include "common/ModuleData.h"
#include "common/Logger.h"
#include "common/HttpClient.h"
#include "common/Payload.h"

#include <sstream>
#include <limits>

// 单测时不使用base64，因其输出不可读
#ifndef ENABLE_COVERAGE
#   define ENCODE_WITH_BASE64
#endif

#ifdef ENCODE_WITH_BASE64
#include "common/Base64.h"
#   define encode(v) Base64::encode((const unsigned char *) (v).c_str(), static_cast<unsigned int>((v).size()))
#   define decode(v) Base64::decode(v)
#else
#include "common/HttpClient.h"
#   define encode HttpClient::UrlEncode
#   define decode HttpClient::UrlDecode
#endif

using namespace std;
namespace common {
#define NONE " "
    const char * const SPLIT_BLANK_STR = " ";
    const char * const ARGUS_MODULE_DATA = "ARGUS_MODULE_DATA";

    std::string MetricData::metricName() const {
        auto it = tagMap.find("metricName");
        return it == tagMap.end() ? "" : it->second;
    }

    bool MetricData::check(int index) const {
        if (tagMap.find("metricName") == tagMap.end()) {
            LogWarn("metric[{}]: invalid metricData without metricName", index);
            return false;
        }
        if (valueMap.find("metricValue") == valueMap.end()) {
            LogWarn("metric[{}]: invalid metricData without metricValue", index);
            return false;
        }
        if (tagMap.find("ns") == tagMap.end()) {
            LogWarn("metric[{}]: invalid metricData without namespace", index);
            return false;
        }
        return true;
    }

    boost::json::object MetricData::toJson() const {
        boost::json::object obj;
        for (const auto &it: tagMap) {
            obj[it.first] = it.second;
        }
        for (const auto &it: valueMap) {
            obj[it.first] = it.second;
        }
        return obj;
    }

    boost::json::object CollectData::toJson() const {
        boost::json::array array;
        array.reserve(this->dataVector.size());
        for (const auto &it: this->dataVector) {
            array.push_back(boost::json::value_from(it.toJson()));
        }
        return boost::json::object{
                {"moduleName", moduleName},
                {"dataVector", array},
        };
    }

    void CollectData::print(std::ostream &os) const {
        os << "moduleName:" << this->moduleName << endl;
        int index = 0;
        for (const MetricData &metricData: this->dataVector) {
            os << "  [" << (index++) << "] " << metricData.metricName() << " ";
            const char *sep = "";
            for (const auto &tag: metricData.tagMap) {
                if (tag.first != "metricName") {
                    os << sep << tag.first << "=" << tag.second;
                    sep = ",";
                }
            }
            for (const auto &value: metricData.valueMap) {
                os << sep << value.first << "=" << ToPayloadString(value.second);
                sep = ",";
            }
            os << endl;
        }
    }

    void ModuleData::convertCollectDataToString(const CollectData &collectData, std::string &content) {
        content = convertCollectDataToString(collectData);
    }

    std::string ModuleData::convertCollectDataToString(const CollectData &collectData) {
        if (collectData.moduleName.empty()) {
            return {};
        }
        stringstream sStream;
        sStream << ARGUS_MODULE_DATA
                << SPLIT_BLANK_STR << collectData.moduleName
                << SPLIT_BLANK_STR << collectData.dataVector.size();

        for (auto &metricData: collectData.dataVector) {
            sStream << SPLIT_BLANK_STR;
            formatMetricData(metricData, sStream);
        }
        return sStream.str();
    }

    bool ModuleData::convertStringToCollectData(const std::string &content, CollectData &collectData, bool check) {
        if (content.empty()) {
            return false;
        }

        bool succeed = [&]() {
            stringstream sStream(content);
            string prefix;
            string moduleName;
            int metricDataNum = 0;
            sStream >> prefix >> moduleName >> metricDataNum;
            if (prefix != ARGUS_MODULE_DATA) {
                LogWarn("invalid prefix: {}", prefix);
                return false;
            }
            if (moduleName.empty()) {
                LogWarn("empty moduleName");
                return false;
            }
            collectData.moduleName = moduleName;
            for (int i = 0; i < metricDataNum; i++) {
                MetricData metricData;
                if (!getMetricData(sStream, metricData)) {
                    LogWarn("metric[{}]: invalid content", i);
                    return false;
                }
                if (check && !metricData.check(i)) {
                    return false;
                }
                collectData.dataVector.push_back(metricData);
            }
            return true;
        }();

        if (!succeed) {
            LogWarn("content: {}", content);
        }

        return succeed;
    }

    std::string ModuleData::convert(const MetricData &metricData) {
        std::stringstream ss;
        formatMetricData(metricData, ss);
        return ss.str();
    }

    void ModuleData::formatMetricData(const MetricData &metricData, std::stringstream &sStream) {
        sStream << metricData.valueMap.size();
        for (auto &it: metricData.valueMap) {
            sStream << SPLIT_BLANK_STR << it.first << SPLIT_BLANK_STR;
            // 小数点后如果无数值的话直接输出为整数，避免使用科学计数法时产生精度损失
            if (it.second >= 100 * 10000 && 0.0 == (it.second - (int64_t) (it.second))) {
                sStream << (int64_t) it.second;
            } else {
                sStream << it.second; // > 1,000,000 时, 可能会有精度损失
            }
        }

        sStream << SPLIT_BLANK_STR << metricData.tagMap.size();
        for (auto const &it1: metricData.tagMap) {
            string tagValue = it1.second;
            if (tagValue.empty()) {
                tagValue = NONE;
            }
            sStream << SPLIT_BLANK_STR << it1.first << SPLIT_BLANK_STR << encode(tagValue);
            // string tagValueBase64 = Base64::encode((const unsigned char *) tagValue.c_str(), tagValue.size());
            // sStream << ENCODE(tagValue);
        }
    }

    const auto &mapSpecialDouble = *new std::map<std::string, double>{
            {"inf",  std::numeric_limits<double>::infinity()},
            {"+inf", std::numeric_limits<double>::infinity()},
            {"-inf", -std::numeric_limits<double>::infinity()},
            {"nan",  std::numeric_limits<double>::quiet_NaN()},
            {"+nan", std::numeric_limits<double>::quiet_NaN()},
            {"-nan", -std::numeric_limits<double>::quiet_NaN()},
    };

    double getSpecialDouble(const std::string &s) {
        auto const it = mapSpecialDouble.find(s);
        return it != mapSpecialDouble.end() ? it->second : std::numeric_limits<double>::quiet_NaN();
    }

    struct Double {
        double value = 0;
    };

    std::istream &operator>>(std::istream &ss, Double &d) {
        char c;
        ss.get(c);
        if (c == ' ') {
            ss.get(c);
        }
        if (ss.unget()) {
            if (('0' <= c && c <= '9') || c == '+' || c == '-') {
                ss >> d.value;
            } else {
                std::string v;
                ss >> v;
                d.value = getSpecialDouble(v);
            }
        }
        return ss;
    }

    bool ModuleData::getMetricData(std::stringstream &ss, MetricData &metricData) {
        // 此处使用fail来判断解析过程是否异常，不能使用good, 具体原因请参见: https://cplusplus.com/reference/ios/ios/good/
        auto fail = [&ss]() { return ss.fail() || ss.bad(); };
        int valueNum = 0;
        ss >> valueNum;
        for (int i = 0; i < valueNum && !fail(); i++) {
            string k;
            Double v;
            ss >> k >> v;
            metricData.valueMap[k] = v.value;
        }

        int tagNum = 0;
        ss >> tagNum;
        for (int i = 0; i < tagNum && !fail(); i++) {
            string tagName;
            string tagValue;
            ss >> tagName >> tagValue;
            tagValue = decode(tagValue);
            if (tagValue == NONE) {
                tagValue = "";
            }
            metricData.tagMap[tagName] = tagValue;
        }

        return !fail();
    }
}
