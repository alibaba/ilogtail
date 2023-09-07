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
#include <unordered_map>
#include "AdhocJobCheckpoint.h"

namespace logtail {

class AdhocCheckpointManager {
private:
    AdhocCheckpointManager() { mRunFlag = false; };
    AdhocCheckpointManager(const AdhocCheckpointManager&) = delete;
    AdhocCheckpointManager& operator=(const AdhocCheckpointManager&) = delete;
    void ProcessLoop();
    static bool mRunFlag;

    std::unordered_map<std::string, AdhocJobCheckpointPtr> mAdhocJobCheckpointMap;
    

public:
    static AdhocCheckpointManager* GetInstance() {
        static AdhocCheckpointManager* ptr = new AdhocCheckpointManager();
        return ptr;
    }

    void Run();
    void LoadAdhocCheckpoint();

    AdhocJobCheckpointPtr GetAdhocJobCheckpoint(const std::string& jobName);
    AdhocJobCheckpointPtr CreateAdhocJobCheckpoint(const std::string& jobName, std::vector<AdhocFileCheckpointKey> adHocFileCheckpointKeyList);
    void DeleteAdhocJobCheckpoint(const std::string& jobName);
};

} // namespace logtail