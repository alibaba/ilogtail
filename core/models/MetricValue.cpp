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
            StringBuffer s = mSourceBuffer->CopyString(itr.key().asString());
            mValues[StringView(s.data, s.size)] = itr->asDouble();
        }
    }
}

Json::Value MetricValueToJson(const MetricValue& value) {
    Json::Value res;
    visit(
        [&](auto&& arg) {
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, UntypedSingleValue>) {
                res["type"] = "untyped_single_value";
                res["detail"] = get<UntypedSingleValue>(value).ToJson();
            } else if constexpr (is_same_v<T, UntypedMultiValues>) {
                res["type"] = "untyped_multi_values";
                res["detail"] = get<UntypedMultiValues>(value).ToJson();
            } else if constexpr (is_same_v<T, monostate>) {
                res["type"] = "unknown";
            }
        },
        value);
    return res;
}

MetricValue JsonToMetricValue(const string& type, const Json::Value& detail) {
    if (type == "untyped_single_value") {
        UntypedSingleValue v;
        v.FromJson(detail);
        return v;
    } else if (type == "untyped_multi_values") {
        UntypedMultiValues v;
        v.mSourceBuffer = std::make_shared<SourceBuffer>();
        v.FromJson(detail);
        return v;
    } else {
        return MetricValue();
    }
}
#endif

} // namespace logtail
