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

#include "prometheus/labels/TextParserSIMD.h"

#include <immintrin.h>

#include <boost/algorithm/string.hpp>
#include <cmath>
#include <string>

#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/StringView.h"
#include "prometheus/Utils.h"

using namespace std;

namespace logtail::prom {


// start to parse metric sample:test_metric{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083 # exemplarsxxx


// parse:test_metric{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083 # exemplarsxxx


// parse:k1="v1", k2="v2" } 9.9410452992e+10 1715829785083 # exemplarsxxx


// parse:"v1", k2="v2" } 9.9410452992e+10 1715829785083 # exemplarsxxx


// parse:v1", k2="v2" } 9.9410452992e+10 1715829785083 # exemplarsxxx

// parse:, k2="v2" } 9.9410452992e+10 1715829785083 # exemplarsxxx
// or parse:} 9.9410452992e+10 1715829785083 # exemplarsxxx


// parse:9.9410452992e+10 1715829785083 # exemplarsxxx


// parse:1715829785083 # exemplarsxxx
// timestamp will be 1715829785.083 in OpenMetrics





} // namespace logtail::prom
