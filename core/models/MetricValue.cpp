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

#include "models/MetricValue.h"

using namespace std;

namespace logtail {

bool UntypedMultiValues::Get(StringView key, double& val) const {
    if (mValues.find(key) != mValues.end()) {
        val = mValues.at(key);
        return true;
    }
    return false;
}

bool UntypedMultiValues::Has(StringView key) const {
    return mValues.find(key) != mValues.end();
}

void UntypedMultiValues::Set(const std::string& key, double val) {
    if (mMetricEventPtr) {
        SetNoCopy(mMetricEventPtr->GetSourceBuffer()->CopyString(key), val);
    }
}

void UntypedMultiValues::Set(StringView key, double val) {
    if (mMetricEventPtr) {
        SetNoCopy(mMetricEventPtr->GetSourceBuffer()->CopyString(key), val);
    }
}

void UntypedMultiValues::SetNoCopy(const StringBuffer& key, double val) {
    SetNoCopy(StringView(key.data, key.size), val);
}

void UntypedMultiValues::SetNoCopy(StringView key, double val) {
    mValues[key] = val;
}

void UntypedMultiValues::Del(StringView key) {
    mValues.erase(key);
}

std::map<StringView, double>::const_iterator UntypedMultiValues::MultiKeyValusBegin() const {
    return mValues.begin();
}

std::map<StringView, double>::const_iterator UntypedMultiValues::MultiKeyValusEnd() const {
    return mValues.end();
}

size_t UntypedMultiValues::MultiKeyValusSize() const {
    return mValues.size();
}

size_t UntypedMultiValues::DataSize() const {
    size_t totalSize = sizeof(UntypedMultiValues);
    for (const auto& pair : mValues) {
        totalSize += pair.first.size() + sizeof(pair.second);
    }
    return totalSize;
}

size_t DataSize(const MetricValue& value) {
    return visit(
        [](auto&& arg) {
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, monostate>) {
                return 0UL;
            } else {
                return arg.DataSize();
            }
        },
        value);
}

#ifdef APSARA_UNIT_TEST_MAIN
Json::Value UntypedSingleValue::ToJson() const {
    return Json::Value(mValue);
}

void UntypedSingleValue::FromJson(const Json::Value& value) {
    mValue = value.asFloat();
}

Json::Value UntypedMultiValues::ToJson() const {
    Json::Value res;
    for (auto metric : mValues) {
        res[metric.first.to_string()] = metric.second;
    }
    return res;
}

void UntypedMultiValues::FromJson(const Json::Value& value) {
    mValues.clear();
    for (Json::Value::const_iterator itr = value.begin(); itr != value.end(); ++itr) {
        if (itr->asDouble()) {
            Set(itr.key().asString(), itr->asDouble());
        }
    }
}
#endif

} // namespace logtail
