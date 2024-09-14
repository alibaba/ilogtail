// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pipeline/plugin/instance/FlusherInstance.h"

#include "monitor/MetricConstants.h"

namespace logtail {
bool FlusherInstance::Init(const Json::Value& config, PipelineContext& context, Json::Value& optionalGoPipeline) {
    mPlugin->SetContext(context);
    mPlugin->SetNodeID(NodeID());
    mPlugin->SetMetricsRecordRef(Name(), PluginID(), NodeID(), ChildNodeID());
    if (!mPlugin->Init(config, optionalGoPipeline)) {
        return false;
    }

    mInEventsCnt = mPlugin->GetMetricsRecordRef().CreateCounter("in_events_cnt");
    mInGroupDataSizeBytes = mPlugin->GetMetricsRecordRef().CreateCounter("in_event_group_data_size_bytes");
    return true;
}

bool FlusherInstance::Send(PipelineEventGroup&& g) {
    mInEventsCnt->Add(g.GetEvents().size());
    mInGroupDataSizeBytes->Add(g.DataSize());
    return mPlugin->Send(std::move(g));
}

} // namespace logtail
