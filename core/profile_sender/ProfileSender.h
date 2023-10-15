/*
 * Copyright 2022 iLogtail Authors
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

#include <string>
#include <vector>
#include "log_pb/sls_logs.pb.h"
#include "common/Lock.h"

namespace logtail {

class ProfileSender {
private:
    void SendRunningStatus(sls_logs::LogGroup& logGroup);

protected:
    SpinLock mProfileLock;
    std::string mDefaultProfileProjectName;
    std::string mDefaultProfileRegion;
    std::unordered_map<std::string, std::string> mAllProfileProjectNames;

    ProfileSender();
    virtual ~ProfileSender() = default;

public:
    ProfileSender(const ProfileSender&) = delete;
    ProfileSender& operator=(const ProfileSender&) = delete;

    static ProfileSender* GetInstance();
    
    void SetDefaultProfileRegion(const std::string& profileRegion);
    std::string GetDefaultProfileRegion();

    void SetDefaultProfileProjectName(const std::string& profileProjectName);
    std::string GetDefaultProfileProjectName();

    std::string GetProfileProjectName(const std::string& region, bool* existFlag = nullptr);
    void GetAllProfileRegion(std::vector<std::string>& allRegion);
    void SetProfileProjectName(const std::string& region, const std::string& profileProject);

    virtual void SendToProfileProject(const std::string& region, sls_logs::LogGroup& logGroup);

    virtual bool SendInstantly(sls_logs::LogGroup& logGroup,
                       const std::string& aliuid,
                       const std::string& region,
                       const std::string& projectName,
                       const std::string& logstore);
    virtual void
    SendToLineCountProject(const std::string& region, const std::string& projectName, sls_logs::LogGroup& logGroup);
};

} // namespace logtail
