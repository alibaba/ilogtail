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
#include <string>
#include <memory>
#include <vector>

#include "json/json.h"

namespace logtail {

struct NewConfig {
    std::string mName;
    uint32_t mCreateTime;
    const Json::Value* mGlobal = nullptr;
    std::vector<const Json::Value*> mInputs;
    std::vector<const Json::Value*> mProcessors;
    std::vector<const Json::Value*> mAggregators;
    std::vector<const Json::Value*> mFlushers;
    std::vector<const Json::Value*> mExtensions;
    bool mHasNativeInput = false;
    bool mHasGoInput = false;
    bool mHasGoProcessor = false;
    bool mHasNativeFlusher = false;
    bool mHasGoFlusher = false;
    bool mIsFirstProcessorJson = false;
    Json::Value mDetail;

    bool Parse();

    bool ShouldNativeFlusherConnectedByGoPipeline() const {
        return mHasGoProcessor || (mHasGoInput && !mHasNativeInput && mProcessors.empty());
    }
    bool IsFlushingThroughGoPipelineExisted() const {
        return mHasGoFlusher || ShouldNativeFlusherConnectedByGoPipeline();
    }
    bool HasGoPlugin() const { return mHasGoFlusher || mHasGoProcessor || mHasGoInput; }
};

} // namespace logtail
