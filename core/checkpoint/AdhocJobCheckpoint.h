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
    std::vector<AdhocFileCheckpointPtr> mAdhocFileCheckpointList;
    int32_t mFileCount;
    int32_t mFilePos;
    std::string mAdhocJobName; 
    bool mDeleteFlag;

public:
    AdhocJobCheckpoint(const std::string& jobName);
    ~AdhocJobCheckpoint();

    AdhocFileCheckpointPtr GetAdhocFileCheckpoint(AdhocFileCheckpointKey adHocFileCheckpointKey);
    int32_t UpdateAdhocFileCheckpoint(AdhocFileCheckpointKey adHocFileCheckpointKey, AdhocFileCheckpointPtr adHocFileCheckpointPtr);
    void DumpAdhocCheckpoint();
    void Delete();

    void AddAdhocFileCheckpoint(AdhocFileCheckpointKey adHocFileCheckpointKey);
};

typedef std::shared_ptr<AdhocJobCheckpoint> AdhocJobCheckpointPtr;

} // namespace logtail
