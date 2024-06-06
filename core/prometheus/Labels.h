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

#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "models/MetricEvent.h"

namespace logtail {

// Label is a key/value pair of strings.
struct Label {
    std::string name;
    std::string value;
    Label(std::string name, std::string value) : name(name), value(value) {}
};

using LabelMap = std::map<std::string, std::string>;
/// @brief Labels is a sorted set of labels. Order has to be guaranteed upon instantiation
class Labels {
private:
    LabelMap labels;

    // 现阶段metricEventPtr永远为空
    MetricEvent* metricEventPtr = nullptr;

public:
    Labels();
    size_t size() const;
    // void Bytes();
    // void MatchLabels();
    // void Hash();
    // void HashForLabels();
    // void HashWithoutLabels();
    // void BytesWithlabels();
    // void BytesWithoutLabels();
    // void Copy();
    // void Has();

    std::string Get(const std::string&);
    void Reset(MetricEvent*);
    void push_back(const Label&);

    void Range(const std::function<void(Label)>&);

    // 提供对内部 set 的迭代器的访问
    // LabelMap::iterator begin() { return labels.begin(); }
    // LabelMap::iterator end() { return labels.end(); }

    // 为常量对象提供只读访问
    LabelMap::const_iterator begin() const;
    LabelMap::const_iterator end() const;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class LabelsUnittest;
#endif
};

class LabelsBuilder {
private:
    Labels base;

    std::vector<std::string> deleteLabelNameList;
    std::vector<Label> addLabelList;

public:
    LabelsBuilder();
    // LabelsBuilder(Labels);
    void DeleteLabel(const std::vector<std::string>&);
    void DeleteLabel(std::string);

    std::string Get(const std::string&);
    void Set(const std::string&, const std::string&);

    void Reset(Labels);
    void Reset(MetricEvent*);

    Labels labels();

    void Range(const std::function<void(Label)>& closure);

#ifdef APSARA_UNIT_TEST_MAIN
    friend class LabelsBuilderUnittest;
#endif
};

} // namespace logtail
