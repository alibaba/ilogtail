/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ProcessorK8sMetadata.h"

#include <vector>
#include <string>
#include "common/ParamExtractor.h"
#include "logger/Logger.h"
#include "models/MetricEvent.h"
#include "models/SpanEvent.h"
#include "monitor/MetricConstants.h"
#include <boost/utility/string_view.hpp>
#include "k8sMetadata.h"

using logtail::StringView; 

namespace logtail {

void ProcessorK8sMetadata::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    EventsContainer& events = logGroup.MutableEvents();
    std::vector<std::string> containerVec;
    std::vector<std::string> remoteIpVec;
    std::vector<size_t>  cotainerNotTag;
    for (size_t rIdx = 0; rIdx < events.size(); ++rIdx) {
        if (!ProcessEvent(events[rIdx], containerVec, remoteIpVec)) {
            cotainerNotTag.push_back(rIdx);
        }
    }
    auto& k8sMetadata = K8sMetadata::GetInstance();
    if (!containerVec.empty()) {
        k8sMetadata.GetByContainerIdsFromServer(containerVec);
    }
    if (!remoteIpVec.empty()) {
        k8sMetadata.GetByIpsFromServer(remoteIpVec);
    }
    for (size_t i = 0; i < cotainerNotTag.size(); ++i) {
        ProcessEvent(events[i], containerVec, remoteIpVec);
    }
    cotainerNotTag.clear();
    containerVec.clear();
    remoteIpVec.clear();
}

bool ProcessorK8sMetadata::ProcessEvent(PipelineEventPtr& e,  std::vector<std::string>& containerVec,  std::vector<std::string>& remoteIpVec) {
    if (!IsSupportedEvent(e)) {
        return true;
    }

    if (e.Is<MetricEvent>()) {
        return ProcessEventForMetric(e.Cast<MetricEvent>(), containerVec, remoteIpVec);
    } else if (e.Is<SpanEvent>()) {
        return ProcessEventForSpan(e.Cast<SpanEvent>(), containerVec, remoteIpVec);
    }

    return true;
}

bool ProcessorK8sMetadata::ProcessEventForSpan(SpanEvent& e, std::vector<std::string>& containerVec, std::vector<std::string>& remoteIpVec) {
    bool res = true;
    
    auto& k8sMetadata = K8sMetadata::GetInstance();
    StringView containerIdViewKey("container.id");
    StringView containerIdView = e.HasTag(containerIdViewKey) ? e.GetTag(containerIdViewKey) : StringView{};
    if (!containerIdView.empty()) {
        std::string containerId(containerIdView);
        std::shared_ptr<k8sContainerInfo> container_info = k8sMetadata.GetInfoByContainerIdFromCache(containerId);
        if (container_info == nullptr) {
            containerVec.push_back(containerId);
            res = false;
        } else {
            e.SetTag("workloadName", container_info->workloadName);
            e.SetTag("workloadKind", container_info->workloadKind);
            e.SetTag("namespace", container_info->k8sNamespace);
            e.SetTag("serviceName", container_info->serviceName);
            e.SetTag("pid", container_info->armsAppId);
        }
    }

    StringView ipView("remote_ip");
    StringView remoteIpView = e.HasTag(ipView) ? e.GetTag(ipView) : StringView{};
    if (!remoteIpView.empty()) {
        std::string remote_ip(remoteIpView);
        std::shared_ptr<k8sContainerInfo> ip_info = k8sMetadata.GetInfoByIpFromCache(remote_ip);
        if (ip_info == nullptr) {
            remoteIpVec.push_back(remote_ip);
            res = false;
        } else {
            e.SetTag("peerWorkloadName", ip_info->workloadName);
            e.SetTag("peerWorkloadKind", ip_info->workloadKind);
            e.SetTag("peerNamespace", ip_info->k8sNamespace);
        }
    }

    return res;
}

bool ProcessorK8sMetadata::ProcessEventForMetric(MetricEvent& e, std::vector<std::string>& containerVec, std::vector<std::string>& remoteIpVec) {
    bool res = true;
    
    auto& k8sMetadata = K8sMetadata::GetInstance();
    StringView containerIdViewKey("container.id");
    StringView containerIdView = e.HasTag(containerIdViewKey) ? e.GetTag(containerIdViewKey) : StringView{};
    if (!containerIdView.empty()) {
        std::string containerId(containerIdView);
        std::shared_ptr<k8sContainerInfo> container_info = k8sMetadata.GetInfoByContainerIdFromCache(containerId);
        if (container_info == nullptr) {
            containerVec.push_back(containerId);
            res = false;
        } else {
            e.SetTag("workloadName", container_info->workloadName);
            e.SetTag("workloadKind", container_info->workloadKind);
            e.SetTag("namespace", container_info->k8sNamespace);
            e.SetTag("serviceName", container_info->serviceName);
            e.SetTag("pid", container_info->armsAppId);
        }
    }

    StringView ipView("remote_ip");
    StringView remoteIpView = e.HasTag(ipView) ? e.GetTag(ipView) : StringView{};
    if (!remoteIpView.empty()) {
        std::string remote_ip(remoteIpView);
        std::shared_ptr<k8sContainerInfo> ip_info = k8sMetadata.GetInfoByIpFromCache(remote_ip);
        if (ip_info == nullptr) {
            remoteIpVec.push_back(remote_ip);
            res = false;
        } else {
            e.SetTag("peerWorkloadName", ip_info->workloadName);
            e.SetTag("peerWorkloadKind", ip_info->workloadKind);
            e.SetTag("peerNamespace", ip_info->k8sNamespace);
        }
    }

    return res;
}

bool ProcessorK8sMetadata::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<MetricEvent>() || e.Is<SpanEvent>();
}

} // namespace logtail
