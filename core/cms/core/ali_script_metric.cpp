#include "ali_script_metric.h"

#include "common/StringUtils.h"
#include "common/Logger.h"
#include "common/SystemUtil.h"
#include "common/PrometheusMetric.h"
#include "common/JsonValueEx.h"
#include "common/Chrono.h"

using namespace std;
using namespace common;

static const string MSG_KEY="MSG";
static const string COLLECTION_FLAG_KEY="collection_flag";
static const string ERROR_INFO_KEY="error_info";

namespace argus
{
AliScriptMetric::AliScriptMetric(const vector <MetricFilterInfo> &metricFilterInfos,
                                 const vector <LabelAddInfo> &labelAddInfos,
                                 const vector <MetricMeta> &metricMetas,
                                 enum ScriptResultFormat resultFormat)
        : BaseParseMetric(metricFilterInfos, labelAddInfos) {

    for (const auto &metricMeta: metricMetas) {
        mMetricMetaMap[metricMeta.name] = metricMeta;
    }
    mResultFormat = resultFormat;
}

AliScriptMetric::~AliScriptMetric() = default;

void AliScriptMetric::AddConstructMetrics(const std::vector<std::pair<std::string, std::string>> &stringLabels,
                                          const std::vector<std::pair<std::string, double>> &numberMetrics) {
    vector<CommonMetric> commonMetrics;
    int64_t timestamp = NowMillis();
    for(const pair<string,double>& metricIt : numberMetrics){
        CommonMetric commonMetric;
        commonMetric.name=metricIt.first;
        commonMetric.value=metricIt.second;
        commonMetric.timestamp=timestamp;
        for(const pair<string,string>& labelIt : stringLabels){
            commonMetric.tagMap[labelIt.first]=labelIt.second;
        }
        AddCommonMetric(commonMetric);
    }
}

enum BaseParseMetric::ParseErrorType AliScriptMetric::AddTextMetric(const string &metricStr){
    LogDebug("AddTextMetric for result:{}",metricStr);
    std::vector<std::string> kvStrs = StringUtils::split(metricStr, " ,", true);
    int64_t timestamp = 0;
    if (!kvStrs.empty() && !convert(kvStrs[0], timestamp)) {
        LogWarn("parse alimonitorText timestamp {} failed!", kvStrs[0]);
        return ALIMONITOR_TEXT_TIMESTAMP_INVALID;
    }
    std::vector<std::pair<std::string, std::string>> stringLabels;
    std::vector<std::pair<std::string, double>> numberMetrics;
    // bool invalidFlag=false;
	for (size_t i = 1; i < kvStrs.size(); i++) {
        std::vector<std::string> kvPairs = StringUtils::split(kvStrs[i], "=", true);
        if(kvPairs.size()!=2){
            LogWarn("a kv string {} can not parse to a kv pair, skip!",kvStrs[i]);
            continue;
        }
        string key=kvPairs[0];
        string value=kvPairs[1];
        map<string,MetricMeta>::iterator metaMapIt;
        double valueNum;
        if((metaMapIt=mMetricMetaMap.find(key))!=mMetricMetaMap.end()){
            MetricMeta meta=metaMapIt->second;
            // 1 for number, 2 for string
            if(meta.type==1){
				if (convert(value, valueNum)) {
                    numberMetrics.emplace_back(key,valueNum);
                }else{
                    // valueNum=0.0;
                    stringLabels.emplace_back(key,value);
                    if(meta.isMetric){
                        numberMetrics.emplace_back(key, 0.0);
                    }
                }
            }else if(meta.type==2){
                stringLabels.emplace_back(key,value);
                if(meta.isMetric){
                    numberMetrics.emplace_back(key,0.0);
                }
            }else{
				if (convert(value, valueNum)) {
                    numberMetrics.emplace_back(key,valueNum);
                }else{
                    if(meta.isMetric){
                        numberMetrics.emplace_back(key,0.0);
                    }
                    stringLabels.emplace_back(key,value);
                }
            }
        }else{
			if (convert(value, valueNum)) {
                numberMetrics.emplace_back(key,valueNum);
            }else{
                stringLabels.emplace_back(key,value);
            }
        }
    }
    AddConstructMetrics(stringLabels,numberMetrics);
    return PARSE_SUCCESS; // invalidFlag? ALIMONITOR_TEXT_KEY_VALUE_PAIR_INVALID: PARSE_SUCCESS;
}
enum BaseParseMetric::ParseErrorType AliScriptMetric::AddJsonMsgObject(const json::Object &msgValue){
    std::vector<std::pair<std::string, std::string>> stringLabels;
    std::vector<std::pair<std::string, double>> numberMetrics;
    // Json::Value::Members members;
    // members = msgValue.getMemberNames();
    bool invalidFlag=false;
    // for (Json::Value::Members::iterator itMember = members.begin(); itMember != members.end(); itMember++){
    for (const auto &itMember: *msgValue.native()) {
        const string key = itMember.key();
        // Json::Value msgItem = msgValue.get(key,Json::Value::null);
        json::Value msgItem{itMember.value()};
        if (msgItem.isNull()) {
            LogWarn("get a MSG member from its key {} but return null, skip!", key);
            invalidFlag = true;
            continue;
        }
        auto metaMapIt = mMetricMetaMap.find(key);
        if (metaMapIt != mMetricMetaMap.end()) {
            const MetricMeta &meta = metaMapIt->second;
            string valueStr;
            double valueNum;
            // 1 for number, 2 for string
            if(meta.type==1){
                if(msgItem.isNumber()){
                    numberMetrics.emplace_back(key, msgItem.asNumber<double>());
                }else if(msgItem.isString()){
                    valueStr = msgItem.asString();
                    if(!convert(valueStr,valueNum)){
                        valueNum=0.0;
                        stringLabels.emplace_back(key,valueStr);
                        if(meta.isMetric){
                            numberMetrics.emplace_back(key,valueNum);
                        }
                    }else{
                        numberMetrics.emplace_back(key,valueNum);
                    }
                }else{
                    invalidFlag=true;
                    LogWarn("a MSG member {}'s value is not double or string, skip!",key);
                    continue;
                }
            }else if(meta.type==2){
                if(msgItem.isNumber()){
                    valueNum = msgItem.asNumber<double>();
                    stringLabels.emplace_back(key,StringUtils::NumberToString(valueNum));
                }else if(msgItem.isString()){
                    valueStr = msgItem.asString();
                    stringLabels.emplace_back(key,valueStr);
                }else{
                    invalidFlag=true;
                    LogWarn("a MSG member {}'s value is not double or string, skip!",key);
                    continue;
                }
                if(meta.isMetric){
                    numberMetrics.emplace_back(key,0.0);
                }
            }else{
                if(msgItem.isNumber()){
                    numberMetrics.emplace_back(key, msgItem.asNumber<double>());
                }else if(msgItem.isString()){
                    valueStr = msgItem.asString();
                    if(meta.isMetric){
						if (!convert(valueStr, valueNum)) {
                            stringLabels.emplace_back(key,valueStr);
                            numberMetrics.emplace_back(key,0.0);
                        }else{
                            numberMetrics.emplace_back(key,valueNum);
                        }
                    }else{
                        stringLabels.emplace_back(key,valueStr);
                    }
                }else{
                    invalidFlag=true;
                    LogWarn("a MSG member {}'s value is not double or string, skip!",key);
                    continue;
                }
            }
        }else{
            if(msgItem.isNumber()){
                numberMetrics.emplace_back(key, msgItem.asNumber<double>());
            }else if(msgItem.isString()){
                stringLabels.emplace_back(key,msgItem.asString());
            }else{
                invalidFlag=true;
                LogWarn("a MSG member {}'s value is not double or string, skip!",key);
                continue;
            }
        }
    }
    AddConstructMetrics(stringLabels,numberMetrics);
    return invalidFlag ? ALIMONITOR_JSON_MSG_MEMBER_INVALID : PARSE_SUCCESS;
}
string AliScriptMetric::StandardizePythonJson(const string &oldStr){
	string newStr = oldStr;
	for (char &ch : newStr) {
		if (ch == '\'') {
            ch = '\"';
		}
	}
    for (int i = static_cast<int>(newStr.size()) - 1; i >= 0; i--) {
		if (newStr[i] == '}' || newStr[i] == ']') {
			i--;
			for (; i >= 0 && newStr[i] == ' '; i--);
			if (i >= 0 && newStr[i] == ',') {
				newStr[i] = ' ';
			}
		}
	}
	return newStr;
}

bool GetJsonValue(const json::Object &root, const std::string &key, double &output) {
    json::Value value = root.getValue(key);
    bool ok = value.isNumber();
    if (ok) {
        output = value.asNumber<double>();
    }
    return ok;
}

bool GetJsonValue(const json::Object &root, const std::string &key, std::string &output) {
    json::Value value = root.getValue(key);
    bool ok = value.isString();
    if (ok) {
        output = value.asString();
    }
    return ok;
}

enum BaseParseMetric::ParseErrorType AliScriptMetric::AddJsonMetric(const string &metricStr){
    LogDebug("AddJsonMetric for result: {}",metricStr);
    std::string error;
    json::Value jsonValue = json::parse(metricStr, error);
    if (jsonValue.isNull()) {
        string tmpStr = StandardizePythonJson(metricStr);
        LogDebug("AddJsonMetric for result: {}", tmpStr);
        error = "";
        jsonValue = json::parse(tmpStr, error);
        if (jsonValue.isNull()) {
            LogError("AliScript result {} error: neither json nor StandardPythonJson", metricStr);
            return ALIMONITOR_JSON_RESULT_NOT_JSON;
        }
    }
    // JsonValue jsonValue;
    // if (jsonValue.parse(metricStr) == false)
    // {
    //     string tmpStr=StandardizePythonJson(metricStr);
    //     LogDebug("AddJsonMetric for result:{}",tmpStr.c_str());
    //     if(jsonValue.parse(tmpStr)==false){
    //         LogError("AliScrpit result {} format error : not json (config to json), please check!",metricStr.c_str());
    //         return ALIMONITOR_JSON_RESULT_NOT_JSON;
    //     }
    // }
    // Json::Value jValue = jsonValue.getValue();
    // if (!jValue.isObject())
    json::Object jValue = jsonValue.asObject();
    if (jValue.isNull()) {
        LogError("AliScript result {} format error : not object", metricStr);
        return ALIMONITOR_JSON_RESULT_NOT_OBJECT;
    }

    double collectionFlag;
    if(GetJsonValue(jValue, COLLECTION_FLAG_KEY, collectionFlag)){
        LabelAddInfo labelAddinfo;
        labelAddinfo.name=COLLECTION_FLAG_KEY;
        labelAddinfo.value=StringUtils::NumberToString(collectionFlag);
        labelAddinfo.type=2;
        mJsonlabelAddInfos.push_back(labelAddinfo);
    }
    string errorInfo;
    if(GetJsonValue(jValue, ERROR_INFO_KEY, errorInfo)){
        LabelAddInfo labelAddinfo;
        labelAddinfo.name=ERROR_INFO_KEY;
        if(collectionFlag==0.0){
            errorInfo="";
        }
        labelAddinfo.value=errorInfo;
        labelAddinfo.type=2;
        mJsonlabelAddInfos.push_back(labelAddinfo);
    }
    // Json::Value msgValue = jValue.get(MSG_KEY,Json::Value::null);
    json::Value msgValue = jValue.getValue(MSG_KEY);
    if (msgValue.isNull() || (!msgValue.isObject() && !msgValue.isArray())) {
        LogWarn("AliScript alimonitorJson output get \"MSG\", but it is null or not an object or array");
        return ALIMONITOR_JSON_MSG_INVALID;
    }
    if(msgValue.isObject()){
        return AddJsonMsgObject(msgValue.asObject());
    }else{
        enum BaseParseMetric::ParseErrorType ret = PARSE_SUCCESS;
        json::Array msgArray = msgValue.asArray();
        // for(int i=0;i<msgValue.size();i++){
        for (size_t i = 0; i < msgArray.size(); i++) {
            // Json::Value msgElementValue = msgValue.get(i, Json::Value::null);
            json::Value msgElementValue = msgArray[i];
            if(msgElementValue.isNull() || !msgElementValue.isObject()){
                LogWarn("AliScrpit alimonitorJson output get \"{}\" a array, but its element is null or not an object", MSG_KEY);
                return ALIMONITOR_JSON_MSG_ARRAY_ELEMENT_INVALID;
            }
            enum BaseParseMetric::ParseErrorType tmpRet = AddJsonMsgObject(msgElementValue.asObject());
            if (ret == PARSE_SUCCESS) {
                ret = tmpRet;
            }
        }
        return ret;
    }
}
void AliScriptMetric::GetJsonLabelAddInfos(vector<LabelAddInfo> &labelAddInfos) const{
    labelAddInfos = mJsonlabelAddInfos;
}
enum BaseParseMetric::ParseErrorType AliScriptMetric::AddMetric(const string &metricStr)
{
    if (metricStr.empty()) {
        return EMPTY_STRING;
    }
    if (mResultFormat == ALIMONITOR_JSON_FORMAT) {
        return AddJsonMetric(metricStr);
    } else {
        return AddTextMetric(metricStr);
    }
}
int AliScriptMetric::AddCommonMetric(const CommonMetric &commonMetric)
{
    string metricName;
    int ret = GetCommonMetricName(commonMetric, mMetricFilterInfoMap,metricName);
    if(ret < 0){
        return -1;
    }
    if(metricName.empty()){
        mCommonMetrics.push_back(commonMetric);
    }else{
        CommonMetric tmpCommonMetric=commonMetric;
        tmpCommonMetric.name=metricName;
        mCommonMetrics.push_back(tmpCommonMetric);
    }
    return 0;
}
string AliScriptMetric::GetMetricKey(const CommonMetric &commonMetric)
{
    string metricKey = commonMetric.name;
    for (const auto & it : commonMetric.tagMap)
    {
        metricKey += it.first + it.second;
    }
    return metricKey;
}

}