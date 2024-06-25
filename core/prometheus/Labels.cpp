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

#include "Labels.h"

#include <algorithm>

using namespace std;
namespace logtail {

Labels::Labels() : metricEventPtr(nullptr) {
}

size_t Labels::size() const {
    if (metricEventPtr) {
        return metricEventPtr->LabelsSize();
    }
    return labels.size();
}

std::string Labels::Get(const string& name) {
    if (metricEventPtr) {
        return string(metricEventPtr->GetTag(name));
    }
    if (labels.count(name)) {
        return labels[name];
    } else {
        return "";
    }
}

/// @brief 从MetricEvent中获取Labels
/// @param metricEvent
/// @warning MetricEvent的name保存在__name__中
void Labels::Reset(MetricEvent* metricEvent) {
    // metricEventPtr = metricEvent;
    for (auto it = metricEvent->LabelsBegin(); it != metricEvent->LabelsEnd(); it++) {
        push_back(Label(it->first.to_string(), it->second.to_string()));
    }
    push_back(Label("__name__", metricEvent->GetName().to_string()));
}

void Labels::push_back(const Label& l) {
    if (metricEventPtr) {
        metricEventPtr->SetTag(l.name, l.value);
        return;
    }
    labels[l.name] = l.value;
}

void Labels::Range(const std::function<void(Label)>& f) {
    if (metricEventPtr) {
        for (auto l = metricEventPtr->LabelsBegin(); l != metricEventPtr->LabelsEnd(); l++) {
            f(Label(string(l->first), string(l->second)));
        }
        return;
    }
    for (auto l : labels) {
        f(Label(l.first, l.second));
    }
}

LabelMap::const_iterator Labels::begin() const {
    // if (metricEventPtr) {
    //     return metricEventPtr->LabelsBegin();
    // } else {
    return labels.begin();
    // }
}

LabelMap::const_iterator Labels::end() const {
    // if (metricEventPtr) {
    //     return metricEventPtr->LabelsEnd();
    // } else {
    return labels.end();
    // }
}


LabelsBuilder::LabelsBuilder() {
}

// Del deletes the label of the given name.
void LabelsBuilder::DeleteLabel(const vector<string>& nameList) {
    for (auto name : nameList) {
        DeleteLabel(name);
    }
}

void LabelsBuilder::DeleteLabel(std::string name) {
    auto it = find_if(addLabelList.begin(), addLabelList.end(), [&name](Label l) { return l.name == name; });
    if (it != addLabelList.end()) {
        addLabelList.erase(it);
    }
    deleteLabelNameList.emplace_back(name);
}

std::string LabelsBuilder::Get(const std::string& name) {
    // Del() removes entries from .add but Set() does not remove from .del, so check .add first.
    for (const Label& label : addLabelList) {
        if (label.name == name) {
            return label.value;
        }
    }
    auto it = find(deleteLabelNameList.begin(), deleteLabelNameList.end(), name);
    if (it != deleteLabelNameList.end()) {
        return "";
    }
    return base.Get(name);
}


// Set the name/value pair as a label. A value of "" means delete that label.
void LabelsBuilder::Set(const std::string& name, const std::string& value) {
    if (value.empty()) {
        DeleteLabel(name);
        return;
    }
    for (size_t i = 0; i < addLabelList.size(); ++i) {
        if (addLabelList[i].name == name) {
            addLabelList[i].value = value;
            return;
        }
    }
    addLabelList.push_back(Label(name, value));
}

void LabelsBuilder::Reset(Labels l) {
    base = l;
    base.Range([this](Label l) {
        if (l.value == "") {
            deleteLabelNameList.push_back(l.name);
        }
    });
}

void LabelsBuilder::Reset(MetricEvent* metricEvent) {
    base.Reset(metricEvent);
    base.Range([this](Label l) {
        if (l.value == "") {
            deleteLabelNameList.push_back(l.name);
        }
    });
}

Labels LabelsBuilder::labels() {
    if (deleteLabelNameList.empty() && addLabelList.empty()) {
        return base;
    }

    auto res = Labels();
    for (const auto& l : base) {
        if (find_if(
                deleteLabelNameList.begin(), deleteLabelNameList.end(), [&l](string name) { return name == l.first; })
                != deleteLabelNameList.end()
            || find_if(addLabelList.begin(), addLabelList.end(), [&l](Label label) { return label.name == l.first; })
                != addLabelList.end()) {
            continue;
        }
        res.push_back(Label(l.first, l.second));
    }
    for (auto l : addLabelList) {
        res.push_back(l);
    }
    // Sort res labels
    // Labels类在push_back后自动排序
    // sort(res.begin(), res.end(), [](Label a, Label b) { return a.name < b.name; });
    return res;
}


/// @brief Range calls f on each label in the Builder
void LabelsBuilder::Range(const std::function<void(Label)>& closure) {
    // Take a copy of add and del, so they are unaffected by calls to Set() or Del().
    auto originAdd = addLabelList;
    auto originDel = deleteLabelNameList;
    base.Range([&originAdd, &originDel, &closure](Label l) {
        if (find_if(originAdd.begin(), originAdd.end(), [&l](Label label) { return l.name == label.name; })
                == originAdd.end()
            && find(originDel.begin(), originDel.end(), l.name) == originDel.end()) {
            closure(l);
        }
    });
    for (auto item : originAdd) {
        closure(item);
    }
}

string Labels::Hash() {
    return to_string(labels.size());
}

void Labels::RemoveMetaLabels() {
    for (auto it = labels.begin(); it != labels.end();) {
        if (it->first.find("__meta_") == 0) {
            it = labels.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace logtail
