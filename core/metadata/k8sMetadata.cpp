// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific l

#include <ctime>
#include <chrono>
#include "k8sMetadata.h"
#include "common/MachineInfoUtil.h"
#include "logger/Logger.h"
#include "common/http/HttpRequest.h"
#include "common/http/HttpResponse.h"


namespace logtail {

    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    void K8sMetadata::FromInfoJson(const Json::Value& json, k8sContainerInfo& info) {
        if (!json.isMember("images") || !json.isMember("labels") ||
            !json.isMember("namespace") ||
            !json.isMember("workloadKind") || !json.isMember("workloadName")) {
            return;
        }

        for (const auto& key : json["images"].getMemberNames()) {
            if (json["images"].isMember(key)) {
                info.images[key] = json["images"][key].asString();
            }
        }
        for (const auto& key : json["labels"].getMemberNames()) {
            if (json["labels"].isMember(key)) {
                info.labels[key] = json["labels"][key].asString();
                if (key == "armsAppId") {
                    info.armsAppId = json["labels"][key].asString();
                }
            }
        }

        info.k8sNamespace = json["namespace"].asString();
        if (json.isMember("serviceName")) {
            info.serviceName = json["serviceName"].asString();
        }
        info.workloadKind = json["workloadKind"].asString();
        info.workloadName = json["workloadName"].asString();
        info.timestamp = std::time(0);

    }

    bool ContainerInfoIsExpired(std::shared_ptr<k8sContainerInfo> info) {
        if (info == nullptr) {
            return false;
        }
        std::time_t now = std::time(0);
        std::chrono::system_clock::time_point th1 = std::chrono::system_clock::from_time_t(info->timestamp);
        std::chrono::system_clock::time_point th2 = std::chrono::system_clock::from_time_t(now);
        std::chrono::duration<double> diff = th2 - th1;
        double minutes_diff = diff.count() / 60.0;
        if (minutes_diff > 10) {
            return true;
        }
        return false;
    }  

    void K8sMetadata::FromContainerJson(const Json::Value& json, std::shared_ptr<ContainerData> data) {
        for (const auto& key : json.getMemberNames()) {
            k8sContainerInfo info;
            FromInfoJson(json[key], info);
            data->containers[key] = info;
        }
    } 

    void K8sMetadata::GetK8sMetadataFromOperator(const std::string& urlHost, const std::string& output, containerInfoType infoType) {
        bool isOk = false;
        isOk = SendRequestToOperator(oneOperatorContainerIdAddr, output, containerInfoType::ContainerIdInfo);
        if (!isOk) {
            LOG_DEBUG(sLogger, ("Failed to send request"));
        }
        return;
    }   

    bool K8sMetadata::SendRequestToOperator(const std::string& urlHost, const std::string& output, containerInfoType infoType) {    
        std::unique_ptr<HttpRequest> request;
        HttpResponse res;
        std::string path = "/metadata/containerid";
        if (infoType == containerInfoType::IpInfo) {
            path = "/metadata/ip"
        }
        request = std::make_unique<HttpRequest>("GET", false, oneOperatorAddr, 9000, path, "", map<string, string>(), "", 30, 3);
        bool success = SendHttpRequest(std::move(request), res);
        if (success) {
            Json::CharReaderBuilder readerBuilder;
            Json::CharReader* reader = readerBuilder.newCharReader();
            Json::Value root;
            std::string errors;

            if (reader->parse(res.mBody.c_str(), res.mBody.c_str() + res.mBody.size(), &root, &errors)) {
                std::shared_ptr<ContainerData> data = std::make_unique<ContainerData>();
                if (data == nullptr) {
                    return true;
                }
                FromContainerJson(root, data); 
                for (const auto& pair : data->containers) {
                    if (infoType == containerInfoType::ContainerIdInfo) {
                        container_cache.insert(pair.first, std::make_shared<k8sContainerInfo>(pair.second));
                    } else {
                        ip_cache.insert(pair.first, std::make_shared<k8sContainerInfo>(pair.second));
                    }
                }
            } else {
                return true;
            }

            delete reader;
        } else {
            LOG_DEBUG(sLogger, ("fetch k8s meta from one operator fail"));
        }
        return true;
    }

    void K8sMetadata::GetByContainerIds(std::vector<std::string> containerIds) {
        Json::Value jsonObj;
        for (auto& str : containerIds) {
            jsonObj["keys"].append(str);
        }
        Json::StreamWriterBuilder writer;
        std::string output = Json::writeString(writer, jsonObj);
        GetK8sMetadataFromOperator(oneOperatorContainerIdAddr, output, containerInfoType::ContainerIdInfo);
    }

    void K8sMetadata::GetByLocalHost() {
        std::string hostIp = GetHostIp();
        std::list<std::string> strList{hostIp};
        Json::Value jsonObj;
        for (const auto& str : strList) {
            jsonObj["keys"].append(str);
        }
        Json::StreamWriterBuilder writer;
        std::string output = Json::writeString(writer, jsonObj);
        std::string urlHost = "http://" + oneOperatorAddr + "/metadata/host";
        GetK8sMetadataFromOperator(urlHost, output, containerInfoType::ContainerIdInfo);
    }

    void K8sMetadata::SetContainerCache(const Json::Value& root) {
        std::shared_ptr<ContainerData> data = std::make_unique<ContainerData>();
        if (data == nullptr) {
            return;
        }
        FromContainerJson(root, data);  
        for (const auto& pair : data->containers) {
           container_cache.insert(pair.first, std::make_shared<k8sContainerInfo>(pair.second));
        }
    }

    void K8sMetadata::SetIpCache(const Json::Value& root) {
        std::shared_ptr<ContainerData> data = std::make_unique<ContainerData>();
        if (data == nullptr) {
            return;
        }
        FromContainerJson(root, data);  
        for (const auto& pair : data->containers) {
           ip_cache.insert(pair.first, std::make_shared<k8sContainerInfo>(pair.second));
        }
    }

    void K8sMetadata::GetByIps(std::vector<std::string> ips) {
        Json::Value jsonObj;
        for (auto& str : ips) {
            jsonObj["keys"].append(str);
        }
        Json::StreamWriterBuilder writer;
        std::string output = Json::writeString(writer, jsonObj);
        std::string urlHost = "http://" + oneOperatorAddr + "/metadata/ip";
        GetK8sMetadataFromOperator(urlHost, output, containerInfoType::IpInfo);
    }

    std::shared_ptr<k8sContainerInfo> K8sMetadata::GetInfoByContainerIdFromCache(const std::string& containerId) {
         if (containerId.empty()) {
            return nullptr;
         }
         return container_cache.get(containerId);
    }
    
    std::shared_ptr<k8sContainerInfo> K8sMetadata::GetInfoByIpFromCache(const std::string& ip) {
        if (ip.empty()){
            return nullptr;
        }
        std::shared_ptr<k8sContainerInfo> ip_info =  ip_cache.get(ip);
        if (ip_info == nullptr) {
            return ip_info;
        }
        if (ContainerInfoIsExpired(ip_info)) {
            return nullptr;
        }
        return ip_info;
    }
    

} // namespace logtail