/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>

#include "container_manager/ContainerDiscoveryOptions.h"
#include "file_server/FileDiscoveryOptions.h"
#include "file_server/MultilineOptions.h"
#include "plugin/interface/Input.h"
#include "reader/FileReaderOptions.h"

namespace logtail {

class InputContainerStdout : public Input {
public:
    static const std::string sName;

    InputContainerStdout();

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    bool Start() override;
    bool Stop(bool isPipelineRemoving) override;

    FileDiscoveryOptions mFileDiscovery;
    bool mEnableContainerDiscovery = false;
    ContainerDiscoveryOptions mContainerDiscovery;
    FileReaderOptions mFileReader;
    MultilineOptions mMultiline;
    bool mIgnoringStdout;
    bool mIgnoringStderr;

private:
    void GenerateContainerMetaFetchingGoPipeline(Json::Value& res) const;
};

} // namespace logtail
