//
// Created by yunjian.ph on 2022/2/11.
//
#include "tcp_collect.h"

#include <fstream>
#include <algorithm>
#include <utility>

#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/TimeProfile.h"
#include "common/Defer.h"
#include "common/Chrono.h"
#include "common/Config.h"

#include "cloud_monitor_const.h"

using namespace std;
using namespace common;
using namespace std::chrono;

namespace cloudMonitor
{
    const string TCP_PREFIX = "Tcp:";

    struct TcpMetricField: FieldName<TcpMetric> {
        const bool check;
        const bool ratio;
        const bool gauge;

        TcpMetricField(const char *name, std::function<double &(TcpMetric &)> fnGet, bool check, bool ratio, bool gauge)
            : FieldName<TcpMetric>(name, std::move(fnGet)), check(check), ratio(ratio), gauge(gauge) {
        }
    };
#define TCP_METRIC_META_ENTRY(N, F, P1, P2, P3) {FIELD_NAME_INITIALIZER(N, TcpMetric, F), P1, P2, P3}
    const TcpMetricField tcpMetricMeta[] = {
            TCP_METRIC_META_ENTRY("ActiveOpens", activeOpens, true, true, true),
            TCP_METRIC_META_ENTRY("PassiveOpens", passiveOpens, true, true, true),
            TCP_METRIC_META_ENTRY("InSegs", inSegs, true, true, true),
            TCP_METRIC_META_ENTRY("OutSegs", outSegs, true, true, true),
            TCP_METRIC_META_ENTRY("EstabResets", estabResets, true, true, true),
            TCP_METRIC_META_ENTRY("AttemptFails", attemptFails, true, true, true),
            TCP_METRIC_META_ENTRY("RetransSegs", retransSegs, true, true, true),
            TCP_METRIC_META_ENTRY("InErrs", inErrs, true, true, true),
            TCP_METRIC_META_ENTRY("OutRsts", outRsts, true, true, true),
            TCP_METRIC_META_ENTRY("CurrEstab", currEstab, false, false, true),
            TCP_METRIC_META_ENTRY("RetransRate", retransRate, false, false, false),
    };
#undef TCP_METRIC_META_ENTRY
    const size_t tcpMetricMetaSize = sizeof(tcpMetricMeta)/sizeof(tcpMetricMeta[0]);
    const TcpMetricField *tcpMetricMetaEnd = tcpMetricMeta + tcpMetricMetaSize;
    static_assert(tcpMetricMetaSize == sizeof(TcpMetric)/sizeof(double), "tcpMetricMeta unexpected");

    void enumerate(const std::function<void(const FieldName<TcpMetric> &)> &callback) {
        for (auto it = tcpMetricMeta; it < tcpMetricMetaEnd; ++it) {
            callback(*it);
        }
    }

template<typename T>
static T delta(const T v1, const T v2)
{
    return v1 > v2 ? v1 - v2 : 0;
}

inline double ratio(const double v1, const double v2)
{
    return v2 != 0 && v1 > 0 ? (1.0 * v1) / v2 : 0;
}

TcpCollect::TcpCollect() {
    LogInfo("load tcp");
}

TcpCollect::~TcpCollect()
{
    LogInfo("unload tcp");
}

int TcpCollect::Init(int totalCount)
{
    mActivate = SingletonConfig::Instance()->GetValue("cms.module.tcp", "false") == "true";
    mTotalCount = totalCount;
    mCount = 0;
    mLastTime = steady_clock::time_point{};
    LogInfo("module tcp activate: {}", mActivate);
    return 0;
}

void TcpCollect::Calculate()
{
    // auto *res = (double *) (&mtcpMetric);
    // auto *last = (double *) (&mLastStat);
    // auto *cur = (double *) (&mCurrentStat);
    double timestamp = (double)duration_cast<seconds>(mCurrentTime - mLastTime).count(); //static_cast<double>(delta(mCurrentTime, mLastTime)) / (1000 * 1000);

    for (auto it = tcpMetricMeta; it < tcpMetricMetaEnd; ++it) {
        if (it->ratio) {
            double value = delta(it->value(mCurrentStat), it->value(mLastStat));
            it->value(mtcpMetric) = ratio(value, timestamp);
        } else if (it->gauge) {
            it->value(mtcpMetric) = it->value(mCurrentStat);
        }
    }
    // for (int i = 0; i < TCP_RATIO_NUM; i++) {
    //     double value = delta(cur[i], last[i]);
    //     res[i] = ratio(value, timestamp);
    // }
    // for (int i = TCP_RATIO_NUM; i < TCP_GAUGE_NUM; i++) {
    //     res[i] = cur[i];
    // }
    mtcpMetric.retransRate = PERCENT(mtcpMetric.retransSegs, mtcpMetric.outSegs);
}

int TcpCollect::ReadSnmpFile(TcpMetric &tcpStat, const string &filePath){
   ifstream fin;
   fin.open(filePath.c_str(),ios_base::in);
   if (fin.fail()){
      LogError("can't open file ({})",filePath.c_str());
      return -1;
   }
   string key;
   bool findKey=false;
   char temp[LEN_4096];
   vector<string> keyVector;
   vector<long> valueVector;
   while (fin.peek() != EOF)
   {
      fin >> key;
      fin.getline(temp, LEN_4096);  
      if(key==TCP_PREFIX){
          string line =string(temp);
          //get key or value line
          if(!findKey){
            //get key line
            findKey =true;
            plugin::PluginUtils::getKeys(line,keyVector);
          }else{
            plugin::PluginUtils::getValues(line,valueVector);
            break;
          }
      }
   }
   fin.close();
   map<string,long>valueMap;
   if(keyVector.size()==valueVector.size()){
       for(int i=0;i<keyVector.size();i++){
           valueMap[keyVector[i]]=valueVector[i];
       }
   }else{
       LogError("keySize({}) !=valueSize({})",keyVector.size(),valueVector.size());
       return -1;
   }
   // //copy value to tcpStat;
   // double * pValue = (double*)(&tcpStat);
   // for(int i=0;i<TCP_GAUGE_NUM;i++){
   //    if(valueMap.find(TCP_METRIC_KEYS[i])!=valueMap.end()){
   //      pValue[i]=valueMap[TCP_METRIC_KEYS[i]];
   //    }else{
   //      pValue[i]=0;
   //    }
   // }
    for (auto it = tcpMetricMeta; it < tcpMetricMetaEnd; ++it) {
        if (it->gauge) {
            auto valueIt = valueMap.find(it->name);
            it->value(tcpStat) = static_cast<double>(valueIt != valueMap.end() ? valueIt->second : 0);
        }
    }
   return 0;
}

void TcpCollect::CheckTcpMetric(TcpMetric &currentMetric,const TcpMetric &lastMetric)
{
    bool exception = false;
    // double *current = (double*)(&currentMetric);
    // double *last = (double*)(&lastMetric);
    // for(int i = 0; i<TCP_CHECK_NUM;i++)
    // {
    //    //skip currEstab
    //    if(current[i]<last[i] && mCheckStatus)
    //    {
    //       current[i] = last[i];
    //       exception = true;
    //       LogInfo("in CheckTcpMetric,i={}",i);
    //    }
    // }
    // if(exception)
    // {
    //    mCheckStatus = false;
    // }else
    // {
    //    mCheckStatus = true;
    // }
    if (mCheckStatus) {
        for (auto it = tcpMetricMeta; mCheckStatus && it < tcpMetricMetaEnd; ++it) {
            if (it->check && it->value(currentMetric) < it->value(lastMetric)) {
                it->value(currentMetric) = it->value(lastMetric);
                exception = true;
                LogInfo("in CheckTcpMetric, i = {}", it - tcpMetricMeta);
            }
        }
    }
    mCheckStatus = !exception;
}

int TcpCollect::Collect(string &data)
{
    if (!mActivate)
    {
        data = "";
        return 0;
    }
    TimeProfile timeProfile;
    mLastStat = mCurrentStat;
    mLastTime = mCurrentTime;
    mCurrentTime = std::chrono::steady_clock::now();
    if (ReadSnmpFile(mCurrentStat,SNMP_FILE) < 0)
    {
        return -1;
    }
    if (mFirstCollect)
    {
        LogInfo("TcpCollect first time");
        // mMetricCalculate = std::make_shared<MetricCalculate<double>>(TCP_METRIC_NUM);
        mFirstCollect = false;
        return 0;
    }
    CheckTcpMetric(mCurrentStat,mLastStat);
    Calculate();

    mMetricCalculate.AddValue(mtcpMetric);

    mCount++;
    LogDebug("collect tcp spend {} ms", timeProfile.millis());
    if (mCount < mTotalCount)
    {
        return 0;
    }

    TcpMetric max, min, avg;
    mMetricCalculate.Stat(max, min, avg);
    GetCollectData(max, min, avg, data);

    mCount = 0;
    mMetricCalculate.Reset();
    return static_cast<int>(data.size());
}

void TcpCollect::GetCollectData(const TcpMetric &max, const TcpMetric &min, const TcpMetric &avg, string &collectStr)
{
    CollectData collectData;
    collectData.moduleName = mModuleName;
    string metricName = "system.tcp.snmp";
    MetricData metricData;
    metricData.tagMap["metricName"] = metricName;
    metricData.tagMap["ns"] = "acs/host";
    metricData.valueMap["metricValue"] = 0;
    for (auto it = tcpMetricMeta; it < tcpMetricMetaEnd; ++it) {
        metricData.valueMap[std::string("max_") + it->name] = it->value(max);
        metricData.valueMap[std::string("min_") + it->name] = it->value(min);
        metricData.valueMap[std::string("avg_") + it->name] = it->value(avg);
    }

    collectData.dataVector.push_back(metricData);
    ModuleData::convertCollectDataToString(collectData, collectStr);
}
}//namespace cloudMonitor

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(tcp) {
    int totalCount = static_cast<int>(ToSeconds(cloudMonitor::kDefaultInterval));
    if (args != nullptr) {
        int count = convert<int>(args);
        if (count > 0) {
            totalCount = count;
        }
    }
    return module::NewHandler<cloudMonitor::TcpCollect>(totalCount);
}
