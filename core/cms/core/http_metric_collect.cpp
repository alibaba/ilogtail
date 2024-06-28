#include "http_metric_collect.h"
#include "http_metric_listen.h"
#include "http_metric_client.h"
#include "argus_manager.h"

#include "common/Config.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/Base64.h"
#include "common/TimeProfile.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/HttpServer.h"
#include "common/CPoll.h"

using namespace std;
using namespace common;

namespace argus
{
HttpMetricCollect::HttpMetricCollect()
{
    mMaxConnectNum = SingletonConfig::Instance()->GetValue("agent.http.metric.max.connect.number",100);
    // mMetrics = NULL;
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    mShennong = std::make_shared<ExporterMetric>(metricFilterInfos, labelAddInfos);
    // mListen = NULL;
    mShennongItem.outputVector.emplace_back("shennong", "");
    LogInfo("agent.http.metric.max.connect.number={}",mMaxConnectNum);
}
HttpMetricCollect::~HttpMetricCollect()
{
    auto listen = mListen.lock();
    if (listen) {
        SingletonPoll::Instance()->remove(listen.get());
        listen->close();
        mListen.reset();
    }
    endThread();
    join();
    // if(mListen !=NULL)
    // {
    //     delete mListen;
    //     mListen =NULL;
    // }
    // if(mShennong != NULL)
    // {
    //     delete mShennong;
    //     mShennong = NULL;
    // }
    // if(mMetrics!=NULL)
    // {
    //     delete mMetrics;
    //     mMetrics =NULL;
    // }
}
void HttpMetricCollect::AddHttpMetricRequest(const std::shared_ptr<HttpMetricClient> &pClient)
{
    if (static_cast<int>(mList.size()) > mMaxConnectNum)
    {
        HttpServerMsg response = HttpServer::CreateHttpServerResponse("request too many in the process queue,please try slowly! ",500);
        pClient->SendResponse(response);
        LogInfo("the http metric request exceeds the max number:{}",mMaxConnectNum);
        return;
    }
    mListLock.lock();
    mList.push_back(pClient);
    mListLock.unlock(); 

}
void HttpMetricCollect::doRun()
{
    TaskManager *pTask = SingletonTaskManager::Instance();
    std::shared_ptr<HttpReceiveItem> httpReceiveItem;
    for (; !isThreadEnd(); sleepFor(1000 * 100)) {
        CpuProfile("HttpMetricCollect::doRun");

        if (pTask->GetHttpReceiveItem().Get(httpReceiveItem)) {
            // mMetricsItem = *prevHttpReceiveItem;
            mMetrics = std::make_shared<ExporterMetric>(
                    httpReceiveItem->metricFilterInfos, httpReceiveItem->labelAddInfos);
        }
        // if(pTask->HttpReceiveItemChanged())
        // {
        //     pTask->GetHttpReceiveItem(mMetricsItem,true);
        //     if(mMetrics !=NULL)
        //     {
        //         delete mMetrics;
        //         mMetrics = NULL;
        //     }
        //     mMetrics = new ExporterMetric(mMetricsItem.metricFilterInfos,mMetricsItem.labelAddInfos);
        // }
        mListLock.lock();
        defer(mListLock.unlock());

        while(!mList.empty())
        {
            std::shared_ptr<HttpMetricClient> pClient = mList.front().lock();
            mList.pop_front();
            if (!pClient) {
                continue;
            }
            map<string,string> tagMap;
            string errorMsg;
            HttpServerMsg response;
            string type;
            if(!ParseHeader(pClient->GetHttpServerMsg().header,type,tagMap,errorMsg))
            {
                response = HttpServer::CreateHttpServerResponse(errorMsg,400);
                pClient->SendResponse(response);
            }else
            {
                response = HttpServer::CreateHttpServerResponse("", 200);
                pClient->SendResponse(response);
                std::shared_ptr<ExporterMetric> exporterMetric;
                vector<pair<string,string>>outputVector;
                if (type == "metrics") {
                    outputVector = httpReceiveItem->outputVector;
                    exporterMetric = mMetrics;

                } else {
                    outputVector = mShennongItem.outputVector;
                    exporterMetric = mShennong;
                }
                if(!exporterMetric)
                {
                    LogDebug("no config for this type={}", type);
                    continue;
                }
                exporterMetric->AddMetricExtra(pClient->GetHttpServerMsg().body,true,&tagMap);
                vector<CommonMetric> commonMetrics;
                exporterMetric->GetCommonMetrics(commonMetrics);
                auto pManager = SingletonArgusManager::Instance()->getChannelManager();
                for(auto & commonMetric : commonMetrics)
                {
                    LogDebug("metric={},value={},tagSize={},timestamp={}", commonMetric.name, commonMetric.value,
                             commonMetric.tagMap.size(), commonMetric.timestamp);
                    if (pManager) {
                        pManager->SendMetricToNextModule(commonMetric, outputVector);
                    }
                }
            }
        }
    }
}
void HttpMetricCollect::Start()
{
    // mListen = new HttpMetricListen();
    // mListen->Listen();
    auto httpListen = std::make_shared<HttpMetricListen>();
    if (httpListen->Listen() == 0) {
        SingletonPoll::Instance()->add(httpListen, httpListen->mNet);
        // LogInfo("add HttpMetricListen to poll successful({}:{})", httpListen->mListenIp, httpListen->mListenPort);
        mListen = httpListen;
    }

    this->runIt();
}
bool HttpMetricCollect::ParseHeader(const string &header,string &type,map<string,string> &tagMap,string &errorMsg)
{
    vector<string> elems = StringUtils::split(header, " ", false);
    if(elems.size()<2)
    {
        errorMsg ="unkown protocol!";
        return false;
    }
    if(elems[0] != "POST" && elems[0] != "PUT")
    {
        errorMsg = string("unsupported request method ")+elems[0];
        LogInfo("unsupported request method({})", elems[0]);
        return false;
    }
    string url = elems[1];
    size_t index =url.find('?');
    if(index !=string::npos)
    {
        url = url.substr(0,index);
    }
    string preUrlMetric = "/metrics";
    string preUrlShennong = "/shennong";
    if(StringUtils::StartWith(url,preUrlMetric))
    {
        type ="metrics";
        url = url.substr(preUrlMetric.size());
    }else if(StringUtils::StartWith(url,preUrlShennong))
    {
        type = "shennong";
        url = url.substr(preUrlShennong.size());
    }
    else
    {
        LogInfo("unsurported url ({})", url);
        errorMsg = string("unsurported url ")+url;
        return false;
    }

    elems = StringUtils::split(url, "/", true);
    if(elems.size()%2 !=0)
    {
        LogInfo("label in the url is invalid:{}", url);
        errorMsg = string("label in the url is invalid: ")+url;
        return false;
    }
    string preKey = "@base64";
	for (size_t i = 1; i < elems.size(); i += 2)
    {
        string key =elems[i-1];
        string value =elems[i];
        index =key.find(preKey);
        if(index != string::npos)
        {
            key = key.substr(0,index);
            value = Base64::decode(value);
        }
        if(!key.empty() && !value.empty())
        {
            tagMap[key] = value;
        }
    }
    return true;
}
}