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

#pragma once
#include <vector>
#include "AdhocFileCheckpoint.h"
#include "common/Lock.h"

namespace logtail {

class AdhocJobCheckpoint {
private:
    bool CheckFileConsistence(const AdhocFileKey* fileKey);
    std::vector<AdhocFileCheckpointPtr> mAdhocFileCheckpointList;
    int32_t mFileCount;
    int32_t mCurrentFileIndex;
    std::string mAdhocJobName;
    ReadWriteLock mRWL;

public:
    AdhocJobCheckpoint(const std::string& jobName);

    void AddFileCheckpoint(AdhocFileCheckpointPtr fileCheckpoint);
    AdhocFileCheckpointPtr GetFileCheckpoint(const AdhocFileKey* fileKey);
    bool UpdateFileCheckpoint(const AdhocFileKey* fileKey, AdhocFileCheckpointPtr fileCheckpoint);

    bool Load(const std::string& path);
    void Dump(const std::string& path);

    bool IsFinished();
    std::string GetJobName();
};

typedef std::shared_ptr<AdhocJobCheckpoint> AdhocJobCheckpointPtr;

} // namespace logtail
