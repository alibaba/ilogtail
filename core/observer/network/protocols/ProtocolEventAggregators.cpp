// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ProtocolEventAggregators.h"

namespace logtail {


void ProtocolEventAggregators::FlushOutMetrics(uint64_t timeNano,
                                               std::vector<sls_logs::Log>& allData,
                                               std::vector<std::pair<std::string, std::string>>& processTags,
                                               std::vector<std::pair<std::string, std::string>>& globalTags,
                                               uint64_t interval) {
    Json::Value root;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = ""; // If you want whitespace-less output
    for (auto& tag : processTags) {
        root[tag.first] = tag.second;
    }
    for (const auto& tag : globalTags) {
        root[tag.first] = tag.second;
    }
    const std::string& tags = Json::writeString(builder, root);

    if (mDNSAggregators != NULL) {
        mDNSAggregators->FlushLogs(allData, tags, interval);
    }

    if (mHTTPAggregators != NULL) {
        mHTTPAggregators->FlushLogs(allData, tags, interval);
    }

    if (mMySQLAggregators != NULL) {
        mMySQLAggregators->FlushLogs(allData, tags, interval);
    }

    if (mRedisAggregators != NULL) {
        mRedisAggregators->FlushLogs(allData, tags, interval);
    }

    if (mPgSQLAggregators != NULL) {
        mPgSQLAggregators->FlushLogs(allData, tags, interval);
    }
}

} // namespace logtail
