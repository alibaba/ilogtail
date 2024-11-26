// Copyright 2024 iLogtail Authors
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

#include "plugin/flusher/local_file/FlusherLocalFile.h"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "pipeline/queue/SenderQueueManager.h"

using namespace std;

namespace logtail {

const string FlusherLocalFile::sName = "flusher_local_file";

bool FlusherLocalFile::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;
    // FileName
    if (!GetMandatoryStringParam(config, "FileName", mFileName, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }
    // MaxFileSize
    GetMandatoryUIntParam(config, "MaxFileSize", mMaxFileSize, errorMsg);
    // MaxFiles
    GetMandatoryUIntParam(config, "MaxFiles", mMaxFileSize, errorMsg);

    // create file writer
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(mFileName, mMaxFileSize, mMaxFiles, true);
    mFileWriter = std::make_shared<spdlog::async_logger>(
        sName, file_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    mFileWriter->set_pattern("[%Y-%m-%d %H:%M:%S.%f] %v");

    mGroupSerializer = make_unique<JsonEventGroupSerializer>(this);
    mSendCnt = GetMetricsRecordRef().CreateCounter(METRIC_PLUGIN_FLUSHER_OUT_EVENT_GROUPS_TOTAL);
    return true;
}

bool FlusherLocalFile::Send(PipelineEventGroup&& g) {
    if (g.IsReplay()) {
        return SerializeAndPush(std::move(g));
    } else {
        vector<BatchedEventsList> res;
        mBatcher.Add(std::move(g), res);
        return SerializeAndPush(std::move(res));
    }
}

bool FlusherLocalFile::Flush(size_t key) {
    BatchedEventsList res;
    mBatcher.FlushQueue(key, res);
    return SerializeAndPush(std::move(res));
}

bool FlusherLocalFile::FlushAll() {
    vector<BatchedEventsList> res;
    mBatcher.FlushAll(res);
    return SerializeAndPush(std::move(res));
}

bool FlusherLocalFile::SerializeAndPush(PipelineEventGroup&& group) {
    string serializedData, errorMsg;
    BatchedEvents g(std::move(group.MutableEvents()),
                    std::move(group.GetSizedTags()),
                    std::move(group.GetSourceBuffer()),
                    group.GetMetadata(EventGroupMetaKey::SOURCE_ID),
                    std::move(group.GetExactlyOnceCheckpoint()));
    mGroupSerializer->DoSerialize(move(g), serializedData, errorMsg);
    if (errorMsg.empty()) {
        mFileWriter->info(serializedData);
    } else {
        LOG_ERROR(sLogger, ("serialize pipeline event group error", errorMsg));
    }
    return true;
}

bool FlusherLocalFile::SerializeAndPush(BatchedEventsList&& groupList) {
    string serializedData;
    for (auto& group : groupList) {
        string errorMsg;
        mGroupSerializer->DoSerialize(move(group), serializedData, errorMsg);
        if (errorMsg.empty()) {
            mFileWriter->info(serializedData);
        } else {
            LOG_ERROR(sLogger, ("serialize pipeline event group error", errorMsg));
        }
    }
    return true;
}

bool FlusherLocalFile::SerializeAndPush(vector<BatchedEventsList>&& groupLists) {
    for (auto& groupList : groupLists) {
        SerializeAndPush(std::move(groupList));
    }
    return true;
}

} // namespace logtail