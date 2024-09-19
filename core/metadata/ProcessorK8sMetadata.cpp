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
    std::vector<std::string> container_vec;
    std::vector<std::string> remote_ip_vec;
    std::vector<size_t>  cotainer_not_tag;
    for (size_t rIdx = 0; rIdx < events.size(); ++rIdx) {
        if (!ProcessEvent(events[rIdx], container_vec, remote_ip_vec)) {
            cotainer_not_tag.push_back(rIdx);
        }
    }
    auto& k8sMetadata = K8sMetadata::GetInstance();
    if (!container_vec.empty()) {
        k8sMetadata.GetByContainerIdsFromServer(container_vec);
    }
    if (!remote_ip_vec.empty()) {
        k8sMetadata.GetByIpsFromServer(remote_ip_vec);
    }
    for (size_t i = 0; i < cotainer_not_tag.size(); ++i) {
        ProcessEvent(events[i], container_vec, remote_ip_vec);
    }
    cotainer_not_tag.clear();
    container_vec.clear();
    remote_ip_vec.clear();
}

bool ProcessorK8sMetadata::ProcessEvent(PipelineEventPtr& e,  std::vector<std::string>& container_vec,  std::vector<std::string>& remote_ip_vec) {
    if (!IsSupportedEvent(e)) {
        return true;
    }

    if (e.Is<MetricEvent>()) {
        return ProcessEventForMetric(e.Cast<MetricEvent>(), container_vec, remote_ip_vec);
    } else if (e.Is<SpanEvent>()) {
        return ProcessEventForSpan(e.Cast<SpanEvent>(), container_vec, remote_ip_vec);
    }

    return true;
}

bool ProcessorK8sMetadata::ProcessEventForSpan(SpanEvent& e, std::vector<std::string>& container_vec, std::vector<std::string>& remote_ip_vec) {
    bool res = true;
    
    auto& k8sMetadata = K8sMetadata::GetInstance();
    StringView container_id_view("container.id");
    StringView containerId_view = e.HasTag(container_id_view) ? e.GetTag(container_id_view) : StringView{};
    if (!containerId_view.empty()) {
        std::string containerId(containerId_view);
        std::shared_ptr<k8sContainerInfo> container_info = k8sMetadata.GetInfoByContainerIdFromCache(containerId);
        if (container_info == nullptr) {
            container_vec.push_back(containerId);
            res = false;
        } else {
            e.SetTag("workloadName", container_info->workloadName);
            e.SetTag("workloadKind", container_info->workloadKind);
            e.SetTag("namespace", container_info->k8sNamespace);
            e.SetTag("serviceName", container_info->serviceName);
            e.SetTag("pid", container_info->armsAppId);
        }
    }

    StringView ip_view("remote_ip");
    StringView remote_ip_view = e.HasTag(ip_view) ? e.GetTag(ip_view) : StringView{};
    if (!remote_ip_view.empty()) {
        std::string remote_ip(remote_ip_view);
        std::shared_ptr<k8sContainerInfo> ip_info = k8sMetadata.GetInfoByIpFromCache(remote_ip);
        if (ip_info == nullptr) {
            remote_ip_vec.push_back(remote_ip);
            res = false;
        } else {
            e.SetTag("peerWorkloadName", ip_info->workloadName);
            e.SetTag("peerWorkloadKind", ip_info->workloadKind);
            e.SetTag("peerNamespace", ip_info->k8sNamespace);
        }
    }

    return res;
}

bool ProcessorK8sMetadata::ProcessEventForMetric(MetricEvent& e, std::vector<std::string>& container_vec, std::vector<std::string>& remote_ip_vec) {
    bool res = true;
    
    auto& k8sMetadata = K8sMetadata::GetInstance();
    StringView container_id_view("container.id");
    StringView containerId_view = e.HasTag(container_id_view) ? e.GetTag(container_id_view) : StringView{};
    if (!containerId_view.empty()) {
        std::string containerId(containerId_view);
        std::shared_ptr<k8sContainerInfo> container_info = k8sMetadata.GetInfoByContainerIdFromCache(containerId);
        if (container_info == nullptr) {
            container_vec.push_back(containerId);
            res = false;
        } else {
            e.SetTag("workloadName", container_info->workloadName);
            e.SetTag("workloadKind", container_info->workloadKind);
            e.SetTag("namespace", container_info->k8sNamespace);
            e.SetTag("serviceName", container_info->serviceName);
            e.SetTag("pid", container_info->armsAppId);
        }
    }

    StringView ip_view("remote_ip");
    StringView remote_ip_view = e.HasTag(ip_view) ? e.GetTag(ip_view) : StringView{};
    if (!remote_ip_view.empty()) {
        std::string remote_ip(remote_ip_view);
        std::shared_ptr<k8sContainerInfo> ip_info = k8sMetadata.GetInfoByIpFromCache(remote_ip);
        if (ip_info == nullptr) {
            remote_ip_vec.push_back(remote_ip);
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
