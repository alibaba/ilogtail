#include "common/ContainerInfoProvider.h"

#include <vector>
#include <fstream>
#include <fmt/printf.h>

#include "common/Config.h"

#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/Common.h"
#include "common/HttpClient.h"
#include "common/JsonValueEx.h"
#include "common/ExecCmd.h"
#include "common/Chrono.h"

using namespace std;

namespace common
{
static const char *POUCH_ID_CMD =  "pouch  ps -q --no-trunc 2>/dev/null";
static const char *DOCKER_ID_CMD = "docker ps -q --no-trunc 2>/dev/null";
static const char *SN_CMD = R"(%s inspect %s |grep '"SN='|awk -F = '{print $2}'|awk -F \" '{print $1}')";
static const char *PID_CMD = "%s inspect %s |grep '\"Pid\":'|awk '{print $2}'|awk -F , '{print $1}'";
static const char *DOCKER = "docker";
static const char *POUCH = "pouch";

ContainerInfoProvider::ContainerInfoProvider()
{
    Config *cfg = SingletonConfig::Instance();
    mUnixSocketPath = "/var/run/docker.sock";
    // curl --unix-socket /var/run/docker.sock 'http://localhost/containers/json'
    mUrl = "http://localhost/containers/json";
    mContainerUrl = "http://localhost/containers";
    cfg->GetValue("agent.docker.update.interval", mInterval, 120);
    mInterval *= (1000 * 1000);
    mLastContainInspectInfoUpdate = 0;
    checkContainEnv();
}
void ContainerInfoProvider::UpdateConfig(const string &unixSocketPath,const string &url,int64_t interval)
{
    mUnixSocketPath =  unixSocketPath;
    mUrl = url;
    auto index = mUrl.find("/json");
    if(index!=string::npos)
    {
        mContainerUrl = mUrl.substr(0,index);
    }
    mInterval = interval;
}
bool ContainerInfoProvider::checkContainEnv()
{
#ifndef WIN32
    std::string commandOutBuffer;

    // FIXME: hancj.2024-04-02 下面的两个命令有时会卡死，导致程序无法完成启动
    if (0 == getCmdOutput(POUCH_ID_CMD, commandOutBuffer))
    {
        mContainerEnv = POUCH_ENV;
        LogDebug("the host is the pouch environment");
        return true;
    }
    else if (0 == getCmdOutput(DOCKER_ID_CMD, commandOutBuffer))
    {
        mContainerEnv = DOCKER_ENV;
        LogDebug("the host is the docker environment");
        return true;
    }
    else
    {
        mContainerEnv = NO_CONTAIN_SUPPORT;
        LogDebug("the host is the no-contain-support environment");
        return false;
    }
#else
	return true;
#endif 
}
bool ContainerInfoProvider::getLiveContainerIds(vector<string> &liveContainerIds)
{
    string cmd;
    // const int maxOutPutLength = 10240;
    // char *commandOutBuffer = new char[maxOutPutLength + 1]{0};
    // ResourceGuard resourceGuard([commandOutBuffer]() {
    //     delete[] commandOutBuffer;
    // });
    if (POUCH_ENV == mContainerEnv) {
        cmd = POUCH_ID_CMD;
    } else if (DOCKER_ENV == mContainerEnv) {
        cmd = DOCKER_ID_CMD;
    } else {
        LogDebug("the host is the no-contain-support environment");
        return false;
    }
    int re = ExecCmd(cmd, liveContainerIds);
    // std::string result;
    // int re = getCmdOutput(cmd, result);
    if (re != 0)
    {
        LogWarn("exec cmd ({}) error", cmd);
        return false;
    }
    // string result = string(commandOutBuffer);
    // StringUtils::splitString(result, "\n", liveContainerIds);
    return true;
}
bool ContainerInfoProvider::getContainerElement(const string & cmd, const string &id,string &result)
{
    string exeCmd;
    if (POUCH_ENV == mContainerEnv) {
        exeCmd = fmt::sprintf(cmd, POUCH, id);
    } else if (DOCKER_ENV == mContainerEnv) {
        exeCmd = fmt::sprintf(cmd, DOCKER, id);
    }

    bool ok = !exeCmd.empty();
    if (ok) {
        std::string buffer;
        if (0 == getCmdOutput(exeCmd, buffer)) {
            result = buffer;
            ok = !result.empty();
        }
    }
    return ok;
}
bool ContainerInfoProvider::updateLiveContainerInfo()
{
    vector<string> liveContainerIds;
    getLiveContainerIds(liveContainerIds);
    for (size_t i = 0; i < liveContainerIds.size(); i++)
    {
        string id = liveContainerIds[i];
        //新的容器，需要刷新其SN和PID信息
        if (mContainerInfoMap.find(id) == mContainerInfoMap.end())
        {
            ContainerInfo dockerInfo;
            dockerInfo.id = id;
            getContainerElement(SN_CMD,id,dockerInfo.sn);
            getContainerElement(PID_CMD,id,dockerInfo.pid);
            mContainerInfoMap[id]=dockerInfo;
            //LogDebug("getContainerInfo:id={},sn={},pid={}",dockerInfo.id.c_str(),dockerInfo.sn.c_str(),dockerInfo.pid.c_str());
        }
    }
    //从map中删除已经不存在的容器信息
    for ( auto it = mContainerInfoMap.begin(); it != mContainerInfoMap.end();)
    {
        bool isLive = false;
        for (const auto & containerId : liveContainerIds)
        {
            if (containerId == it->first)
            {
                isLive = true;
                break;
            }
        }
        if (!isLive)
        {
            auto iterErase = it;
            it++;
            mContainerInfoMap.erase(iterErase);
        }
        else
        {
            it++;
        }
    }
	return true;
}
bool ContainerInfoProvider::getLiveContainerInfo(map<string,ContainerInfo> &dockerInfoMap)
{
    LockGuard lockGuard(mContainerInfoMapLock);
    int64_t currentTime = NowSeconds();
    if (currentTime >= mLastContainerInfoUpdateTime + mInterval / (1000 * 1000))
    {
        updateLiveContainerInfo();
        mLastContainerInfoUpdateTime = currentTime;
    }
    dockerInfoMap = mContainerInfoMap;
    //printContainerInfo(dockerInfoMap);
	return true;
}
bool ContainerInfoProvider::printContainerInfo(map<string,ContainerInfo> &dockerInfoMap)
{
    map<string,ContainerInfo>::iterator it = dockerInfoMap.begin();
    for(;it!=dockerInfoMap.end();it++)
    {
        ContainerInfo dockerInfo = it->second;
        //LogDebug("containerInfo:id={},sn={},pid={}",dockerInfo.id.c_str(),dockerInfo.sn.c_str(),dockerInfo.pid.c_str());
    }
	return true;
}
int ContainerInfoProvider::GetContainInspectInfos(vector<ContainerInspectInfo> & containerInspectInfos,bool forceUpdate)
{
    mContainerInspectInfoLock.lock();
    if (!forceUpdate && NowMicros() <= mLastContainInspectInfoUpdate + mInterval)
    {
        containerInspectInfos = mContainerInspectInfos;
        mContainerInspectInfoLock.unlock();
        return static_cast<int>(containerInspectInfos.size());
    }
    mContainerInspectInfoLock.unlock();
    HttpRequest httpRequest;
    httpRequest.url = mUrl;
    httpRequest.unixSocketPath = mUnixSocketPath;
    HttpResponse httpResponse;
    vector<ContainerInspectInfo> tmpContainerInspectInfos;
    int re = HttpClient::HttpCurl(httpRequest, httpResponse);
    if(re==HttpClient::Success && httpResponse.resCode ==200)
    {
        ParseDockerInspectInfos(httpResponse.result,tmpContainerInspectInfos);
    }else
    {
        LogInfo("curl {} with {},re={},responseCode={},errorMsg={}",
            mUrl, mUnixSocketPath, re, httpResponse.resCode, httpResponse.errorMsg);
    }
    mContainerInspectInfoLock.lock();
    mLastContainInspectInfoUpdate = NowMicros();
    mContainerInspectInfos = tmpContainerInspectInfos;
    containerInspectInfos = mContainerInspectInfos;
    mContainerInspectInfoLock.unlock();
    return static_cast<int>(containerInspectInfos.size());
}
int ContainerInfoProvider::ParseDockerInspectInfos(const string &json,vector<ContainerInspectInfo> & containerInspectInfos)
{
    // Json::Value value;
    // if(!CommonJson::GetJsonArrayValue(json,value))
    // {
    //     LogWarn("docker inspect info not json or array:{}",json.c_str());
    //     return -1;
    // }
    std::string error;
    json::Array value = json::parseArray(json, error);
    if (value.isNull()) {
        LogWarn("docker inspect info not json or array: {}", json);
        return -1;
    }

    value.forEach([&](size_t, const json::Object &item){
        ContainerInspectInfo containerInspectInfo;

        item.get("Id", containerInspectInfo.id);
        if (containerInspectInfo.id.empty()) {
            return;
        }
        //继续获取其他信息
        if (!GetContainerInfo(containerInspectInfo.id, containerInspectInfo.containerInfo)) {
            LogWarn("get container-pid error: {}", containerInspectInfo.id);
        }

        //继续获取cgroup信息
        std::string cgroupFile = "/proc/" + containerInspectInfo.containerInfo.pid + "/cgroup";
        if (!GetContainerCgroupInfo(cgroupFile, containerInspectInfo.cgroupInfo)) {
            LogWarn("get container-cgroup error: {}", containerInspectInfo.id);
        }

        vector<string> names;
        // CommonJson::GetJsonStringValues(item,"Names",names);
        item.getArray("Names").toStringArray(names);
        if (!names.empty()) {
            //只取第一name元素，并且去掉/
            containerInspectInfo.name = names[0].substr(1);
        }

        json::GetMap(item, "Labels", containerInspectInfo.labelMap);
        // Json::Value mounts;
        // if(CommonJson::GetJsonArrayValue(item,"Mounts",mounts))
        // {
        //     for(size_t m=0;m<mounts.size();m++)
        //         Json::Value mountItem = mounts.get(m, Json::Value::null);
        //         if(mountItem.isNull()||!item.isObject())
        //         {
        //             continue;
        //         }
        item.getArray("Mounts").forEach([&](size_t, const json::Object &mountItem) {
            ContainerMountInfo containerMountInfo;
            mountItem.get("Source", containerMountInfo.source);
            mountItem.get("Destination", containerMountInfo.dest);
            containerInspectInfo.mounts.push_back(containerMountInfo);
        });
        // }
        containerInspectInfos.push_back(containerInspectInfo);
    });
    // for(size_t i=0;i<value.size();i++)
    // {
    //     ContainerInspectInfo containerInspectInfo;
    //     Json::Value item = value.get(i, Json::Value::null);
    //     if(item.isNull()||!item.isObject())
    //     {
    //         continue;
    //     }
    //     CommonJson::GetJsonStringValue(item, "Id", containerInspectInfo.id);
    //     if(containerInspectInfo.id.empty())
    //     {
    //         continue;
    //     }
    //     //继续获取其他信息
    //     if(!GetContainerInfo(containerInspectInfo.id,containerInspectInfo.containerInfo))
    //     {
    //        LogWarn("get container-pid error:{}",containerInspectInfo.id.c_str());
    //     }
    //     //继续获取cgroup信息
    //     string cgroupFile = "/proc/"+containerInspectInfo.containerInfo.pid+"/cgroup";
    //     if(!GetContainerCgroupInfo(cgroupFile,containerInspectInfo.cgroupInfo))
    //     {
    //        LogWarn("get container-cgroup error:{}",containerInspectInfo.id.c_str());
    //     }
    //     vector<string> names;
    //     CommonJson::GetJsonStringValues(item,"Names",names);
    //     if(names.size()>0)
    //     {
    //         //只取第一name元素，并且去掉/
    //         containerInspectInfo.name = names[0].substr(1);
    //     }
    //     CommonJson::GetJsonStringMap(item,"Labels",containerInspectInfo.labelMap);
    //     Json::Value mounts;
    //     if(CommonJson::GetJsonArrayValue(item,"Mounts",mounts))
    //     {
    //         for(size_t m=0;m<mounts.size();m++)
    //         {
    //             ContainerMountInfo containerMountInfo;
    //             Json::Value mountItem = mounts.get(m, Json::Value::null);
    //             if(mountItem.isNull()||!item.isObject())
    //             {
    //                 continue;
    //             }
    //             CommonJson::GetJsonStringValue(mountItem, "Source", containerMountInfo.source);
    //             CommonJson::GetJsonStringValue(mountItem, "Destination", containerMountInfo.dest);
    //             containerInspectInfo.mounts.push_back(containerMountInfo);
    //         }
    //     }
    //     containerInspectInfos.push_back(containerInspectInfo);
    // }
    return static_cast<int>(containerInspectInfos.size());
}

string ContainerInfoProvider::GetContainerNodePath(const vector<string> &dockerIds,const string &dockerPath)
{
    vector<ContainerInspectInfo> containerInspectInfos;
    GetContainInspectInfos(containerInspectInfos);
    for(auto idIt = dockerIds.begin(); idIt !=  dockerIds.end();idIt++)
    {
        for(auto cIt = containerInspectInfos.begin(); cIt != containerInspectInfos.end();cIt++)
        {
            if(cIt->id == *idIt)
            {
                for(auto mIt = cIt->mounts.begin();mIt !=  cIt->mounts.end(); mIt++)
                {
                    if(!mIt->dest.empty() && StringUtils::StartWith(dockerPath,mIt->dest))
                    {
                        if(dockerPath==mIt->dest)
                        {
                            return mIt->source;
                        }
                        string suffix = dockerPath.substr(mIt->dest.size());
                        #ifdef win32
                            char split = '\\';
                        #else
                            char split = '/';
                        #endif 
                        if(suffix[0]==split ||mIt->dest[mIt->dest.size()-1]==split)
                        {
                            return string(mIt->source+suffix);
                        }
                    }
                }
            }
        }
    }
    return "";
}
bool ContainerInfoProvider::GetContainerNodePath(const vector<string> &dockerIds,const string &dockerPath, ContainerNodePath &containerNodePath)
{
    vector<ContainerInspectInfo> containerInspectInfos;
    GetContainInspectInfos(containerInspectInfos);
    for(auto idIt = dockerIds.begin(); idIt !=  dockerIds.end();idIt++)
    {
        for(auto cIt = containerInspectInfos.begin(); cIt != containerInspectInfos.end();cIt++)
        {
            if(cIt->id == *idIt)
            {
                for(auto mIt = cIt->mounts.begin();mIt !=  cIt->mounts.end(); mIt++)
                {
                    if(!mIt->dest.empty() && StringUtils::StartWith(dockerPath,mIt->dest))
                    {
                        if(dockerPath==mIt->dest)
                        {
                            containerNodePath.mountInfo.dest=mIt->dest;
                            containerNodePath.mountInfo.source=mIt->source;
                            containerNodePath.file=mIt->source;
                            containerNodePath.dockerFile=dockerPath;
                            return true;;
                        }
                        string suffix = dockerPath.substr(mIt->dest.size());
						if (HasPrefixAny(suffix, { "/", "\\" }) || HasSuffixAny(mIt->dest, { "/", "\\" }))
                        {
                            containerNodePath.mountInfo.dest=mIt->dest;
                            containerNodePath.mountInfo.source=mIt->source;
                            containerNodePath.file=mIt->source+suffix;
                            containerNodePath.dockerFile=dockerPath;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}
bool ContainerInfoProvider::GetContainerInfo(const string &containerId,ContainerInfo & containerInfo)
{
    HttpRequest httpRequest;
    httpRequest.url = mContainerUrl+"/"+containerId+"/json";
    httpRequest.unixSocketPath = mUnixSocketPath;
    HttpResponse httpResponse;
    vector<ContainerInspectInfo> tmpContainerInspectInfos;
    int re = HttpClient::HttpCurl(httpRequest, httpResponse);
    if(re==HttpClient::Success && httpResponse.resCode ==200)
    {
        return ParseContainerInfo(httpResponse.result,containerInfo);
    }else
    {
        LogInfo("curl {} with {},re={},responseCode={},errorMsg={}",
        httpRequest.url, mUnixSocketPath, re, httpResponse.resCode,httpResponse.errorMsg);
        return false;
    }
}
bool ContainerInfoProvider::ParseContainerInfo(const string & json,ContainerInfo & containerInfo)
{
    // Json::Value value;
    // if(!CommonJson::GetJsonObjectValue(json,value))
    // {
    //     LogWarn("container info not json object:{}",json.c_str());
    //     return false;
    // }
    std::string error;
    json::Object value = json::parseObject(json, error);
    if (value.isNull()) {
        LogWarn("container info not json object:{}", json);
        return false;
    }

    value.get("Id", containerInfo.id);
    // Json::Value stateValue;
    // if(!CommonJson::GetJsonObjectValue(value, "State",stateValue))
    // {
    //     LogWarn("container info have no State object:{}",json.c_str());
    //     return false;
    // }
    json::Object stateValue = value.getObject("State");
    if (stateValue.isNull()) {
        LogWarn("container info have no State object: {}", json);
        return false;
    }

    int pid = stateValue.getNumber<int>("Pid", -1);
    if (pid > 0) {
        containerInfo.pid = StringUtils::NumberToString(pid);
        return true;
    } else {
        return false;
    }
}
bool ContainerInfoProvider::GetContainerCgroupInfo(const string & file,ContainerCgroupInfo & containerCgroupInfo)
{
    char temp[4096];
    ifstream fin;
    fin.open(file.c_str(), ios_base::in);
    if (fin.fail())
    {
        LogWarn("open file({}) error",file.c_str());
        return false;
    }
    while (fin.peek() != EOF)
    {
        fin.getline(temp, 4096);
        string content = string(temp);
        size_t index = content.find_last_of(':');
        if(index==string::npos)
        {
            continue;
        }
        string path = content.substr(index+1);
        content = content.substr(0,index);
        if (content.find("cpuacc") != string::npos)
        {
            containerCgroupInfo.cpuacc = path;
        }
        if (content.find("memory") != string::npos)
        {
            containerCgroupInfo.memory = path;
        }
        if (content.find("blkio") != string::npos)
        {
            containerCgroupInfo.blkio = path;
        }
        if (content.find("net_cls") != string::npos)
        {
           containerCgroupInfo.net_cls = path;
        }
    }
    fin.close();
    return true;
}

}