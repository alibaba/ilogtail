/*
 * Copyright 2024 iLogtail Authors
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

#include <sstream>
#include <chrono>
#include <string>
#include <cmath>
#include "re2/re2.h"
#include "models/MetricEvent.h"
#include "prom/TextParser.h"

using namespace std;

namespace prom {

unique_ptr<PipelineEventGroup> TextParser::Parse(const string& content) {
    auto now = std::chrono::system_clock::now();
    auto duration_since_epoch = now.time_since_epoch();
    auto seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch);
    std::time_t timestamp = seconds_since_epoch.count();
    return Parse(content, timestamp);
}

unique_ptr<PipelineEventGroup> TextParser::Parse(const string& content, const time_t defaultTs) {
    string line;
    string argName, argLabels, argUnwrappedLabels, argValue, argSuffix, argTimestamp;
    istringstream iss(content);
    auto eGroup = make_unique<PipelineEventGroup>(mSourceBuffer);
    while (getline(iss, line)) {
        // trim line
        line.erase(0, line.find_first_not_of(" \n\r\t"));
        line.erase(line.find_last_not_of(" \n\r\t")+1);

        // skip any empty line
        if (line.empty()) {
            continue;
        }

        // skip any comment
        if (line[0] == '#') {
            continue;
        }

        // parse line
        // for given sample R"""(test_metric{k1="v1", k2="v2"} 9.9410452992e+10 1715829785083)"""
        // argName = "test_metric"
        // argLabels = R"""({"k1="v1", k2="v2"})"""
        // argUnwrappedLabels = R"""(k1="v1", k2="v2")"""
        // argValue = "9.9410452992e+10"
        // argSuffix = " 1715829785083"
        // argTimestamp = "1715829785083"
        RE2::FullMatch(line, mSampleRegex, RE2::Arg(&argName), RE2::Arg(&argLabels), RE2::Arg(&argUnwrappedLabels), RE2::Arg(&argValue), RE2::Arg(&argSuffix), RE2::Arg(&argTimestamp));

        // skip any sample that has no name
        if (argName.empty()) {
            continue;
        }

        // skip any sample that has a NaN value
        double value;
        try {
            value = stod(argValue);
        } catch (const exception&) {
            continue;
        }
        if (isnan(value)) {
            continue;
        }

        // set timestamp to `defaultTs` if timestamp is empty, otherwise parse it
        // if timestamp is not empty but not a valid integer, skip it
        time_t timestamp;
        if (argTimestamp.empty()) {
            timestamp = defaultTs;
        } else {
            try {
                timestamp = stol(argTimestamp);
            } catch (const exception&) {
                continue;
            }
        }

        auto e = eGroup->AddMetricEvent();
        e->SetName(argName);
        e->SetTimestamp(timestamp);
        auto singleValue = UntypedSingleValue{ value };
        e->SetValue(singleValue);

        if (!argUnwrappedLabels.empty()) {
            string kvPair;
            istringstream iss(argUnwrappedLabels);
            while (getline(iss, kvPair, ',')) {
                kvPair.erase(0, kvPair.find_first_not_of(" "));
                kvPair.erase(kvPair.find_last_not_of(" ")+1);

                size_t equalsPos = kvPair.find('=');
                if (equalsPos != string::npos) {
                    string key = kvPair.substr(0, equalsPos);
                    string value = kvPair.substr(equalsPos + 1);
                    value.erase(0, value.find_first_not_of("\""));
                    value.erase(value.find_last_not_of("\"")+1);
                    e->SetTag(key, value);
                }
            }
        }
    }

    return eGroup;
}

}
