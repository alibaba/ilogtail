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

#include "route/Router.h"

#include "common/ParamExtractor.h"
#include "pipeline/Pipeline.h"
#include "plugin/interface/Flusher.h"

using namespace std;

namespace logtail {

bool Router::Init(std::vector<pair<size_t, const Json::Value*>> configs, const PipelineContext& ctx) {
    for (auto& item : configs) {
        mConditions.emplace_back(item.first, Condition());
        if (!mConditions.back().second.Init(*item.second, ctx)) {
            return false;
        }
    }
    return true;
}

vector<size_t> Router::Route(const PipelineEventGroup& g) const {
    vector<size_t> res;
    for (size_t i = 0, condIdx = 0; i < mFlusherCnt; ++i) {
        if (condIdx < mConditions.size() && mConditions[condIdx].first == i) {
            if (mConditions[condIdx].second.Check(g)) {
                res.push_back(i);
            }
            ++condIdx;
        } else {
            res.push_back(i);
        }
    }
    return res;
}

} // namespace logtail
