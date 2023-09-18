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

namespace logtail {

class AdhocJobCheckpoint {
private:
    bool CheckFileInList(const AdhocFileCheckpointKey* adhocFileCheckpointKey);
    std::vector<AdhocFileCheckpointPtr> mAdhocFileCheckpointList;
    int32_t mFileCount;
    int32_t mCurrentFileIndex;
    std::string mAdhocJobName; 
    std::mutex mMutex;

public:
    AdhocJobCheckpoint(const std::string& jobName);
    ~AdhocJobCheckpoint();

    AdhocFileCheckpointPtr GetAdhocFileCheckpoint(const AdhocFileCheckpointKey* adhocFileCheckpointKey);
    bool UpdateAdhocFileCheckpoint(const AdhocFileCheckpointKey* adhocFileCheckpointKey, AdhocFileCheckpointPtr adhocFileCheckpointPtr);
    bool LoadAdhocCheckpoint(const std::string& path);
    void DumpAdhocCheckpoint(const std::string& path);

    void AddAdhocFileCheckpoint(const AdhocFileCheckpointKey* adhocFileCheckpointKey);
    int32_t GetCurrentFileIndex();
    std::string GetJobName();
    std::vector<std::string> GetFileList();
};

typedef std::shared_ptr<AdhocJobCheckpoint> AdhocJobCheckpointPtr;

} // namespace logtail
