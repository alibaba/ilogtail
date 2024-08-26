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

#include <json/json.h>

class AggregatorDefault {
public:
    static AggregatorDefault& Instance() {
        static AggregatorDefault instance; // Guaranteed to be destroyed and instantiated on first use
        return instance;
    }

    Json::Value* GetJsonConfig() { return &aggregatorDefault; }

private:
    Json::Value aggregatorDefault;
    AggregatorDefault() { aggregatorDefault["Type"] = "aggregator_default"; }

    AggregatorDefault(AggregatorDefault const&) = delete;
    void operator=(AggregatorDefault const&) = delete;
};