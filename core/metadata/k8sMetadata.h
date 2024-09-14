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
#pragma once
#include <iostream>
#include <string>
#include <curl/curl.h>
#include "common/LRUCache.h"
#include <json/json.h>

namespace logtail {
    struct k8sContainerInfo {
        std::unordered_map<std::string, std::string> images;
        std::unordered_map<std::string, std::string> labels;
        std::string k8sNamespace;
        std::string serviceName;
        std::string workloadKind;
        std::string workloadName;
        std::time_t timestamp;
        std::string armsAppId;
    };

    // 定义顶层的结构体
    struct ContainerData {
        std::unordered_map<std::string, k8sContainerInfo> containers;
    };

    
    enum class containerInfoType {
        ContainerIdInfo,
        IpInfo
    };

    class K8sMetadata {
        private:
            lru11::Cache<std::string, std::shared_ptr<k8sContainerInfo>> container_cache;
            lru11::Cache<std::string, std::shared_ptr<k8sContainerInfo>> ip_cache;
            std::string oneOperatorAddr;
            std::string oneOperatorContainerIdAddr;
            K8sMetadata(size_t cacheSize, const std::string& operatorAddr)
              : container_cache(cacheSize, 0), ip_cache(cacheSize, 0), oneOperatorAddr(operatorAddr) {
                //GetByLocalHost();
                oneOperatorContainerIdAddr = oneOperatorAddr;
              }
            K8sMetadata(const K8sMetadata&) = delete;
            K8sMetadata& operator=(const K8sMetadata&) = delete;

        public:
            static K8sMetadata& GetInstance(size_t cacheSize = 500, const std::string& operatorAddr = "oneoperator") {
                static K8sMetadata instance(cacheSize, operatorAddr);
                return instance;
            }

            // 公共方法
            void GetByContainerIds(std::vector<std::string> containerIds);
            void GetByLocalHost();
            void GetByIps(std::vector<std::string> ips);
            std::shared_ptr<k8sContainerInfo> GetInfoByContainerIdFromCache(const std::string& containerId);
            std::shared_ptr<k8sContainerInfo> GetInfoByIpFromCache(const std::string& ip);
            bool SendRequestToOperator(const std::string& urlHost, const std::string& output, containerInfoType infoType);
            void GetK8sMetadataFromOperator(const std::string& urlHost, const std::string& output, containerInfoType infoType);
            void SetIpCache(const Json::Value& root);
            void SetContainerCache(const Json::Value& root);
            void FromInfoJson(const Json::Value& json, k8sContainerInfo& info);
            void FromContainerJson(const Json::Value& json, std::shared_ptr<ContainerData> data);
    };  
    
} // namespace logtail
