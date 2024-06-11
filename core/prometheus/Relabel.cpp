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

#include "Relabel.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/regex.hpp>
#include <iomanip>
#include <sstream>
#include <string>

#include "ParamExtractor.h"
#include "openssl/md5.h"

using namespace std;

#define ENUM_TO_STRING_CASE(EnumValue) \
    { Action::EnumValue, #EnumValue }

#define STRING_TO_ENUM__CASE(EnumValue) \
    { #EnumValue, Action::EnumValue }

namespace logtail {

Action StringToAction(string action) {
    static std::map<string, Action> actionStrings{STRING_TO_ENUM__CASE(replace),
                                                  STRING_TO_ENUM__CASE(keep),
                                                  STRING_TO_ENUM__CASE(drop),
                                                  STRING_TO_ENUM__CASE(keepequal),
                                                  STRING_TO_ENUM__CASE(dropequal),
                                                  STRING_TO_ENUM__CASE(hashmod),
                                                  STRING_TO_ENUM__CASE(labelmap),
                                                  STRING_TO_ENUM__CASE(labeldrop),
                                                  STRING_TO_ENUM__CASE(labelkeep),
                                                  STRING_TO_ENUM__CASE(lowercase),
                                                  STRING_TO_ENUM__CASE(uppercase)};

    auto it = actionStrings.find(action);
    if (it != actionStrings.end()) {
        return it->second;
    }
    return Action::undefined;
}

std::string ActionToString(Action action) {
    static std::map<Action, std::string> actionStrings{ENUM_TO_STRING_CASE(replace),
                                                       ENUM_TO_STRING_CASE(keep),
                                                       ENUM_TO_STRING_CASE(drop),
                                                       ENUM_TO_STRING_CASE(keepequal),
                                                       ENUM_TO_STRING_CASE(dropequal),
                                                       ENUM_TO_STRING_CASE(hashmod),
                                                       ENUM_TO_STRING_CASE(labelmap),
                                                       ENUM_TO_STRING_CASE(labeldrop),
                                                       ENUM_TO_STRING_CASE(labelkeep),
                                                       ENUM_TO_STRING_CASE(lowercase),
                                                       ENUM_TO_STRING_CASE(uppercase)};

    auto it = actionStrings.find(action);
    if (it != actionStrings.end()) {
        return it->second;
    }
    return "undefined";
}

RelabelConfig::RelabelConfig() {
}

RelabelConfig::RelabelConfig(const Json::Value& config) {
    string errorMsg;
    // Json::Value sourceLabelsJson = config["source_labels"];
    // if (sourceLabelsJson.isArray() && sourceLabelsJson.size() > 0) {
    //     for (const auto& item : sourceLabelsJson) {
    //         sourceLabels.emplace_back(item);
    //     }
    // }

    if (config.isMember("source_labels") && config["source_labels"].isArray()) {
        GetOptionalListParam<string>(config, "source_labels", sourceLabels, errorMsg);
    }

    if (config.isMember("separator") && config["separator"].isString()) {
        separator = config["separator"].asString();
    }

    if (config.isMember("target_label") && config["target_label"].isString()) {
        targetLabel = config["target_label"].asString();
    }

    if (config.isMember("regex") && config["regex"].isString()) {
        string re = config["regex"].asString();
        regex = boost::regex(re);
    }

    if (config.isMember("replacement") && config["replacement"].isString()) {
        replacement = config["replacement"].asString();
    }

    if (config.isMember("action") && config["action"].isString()) {
        string actionString = config["action"].asString();
        action = StringToAction(actionString);
    }

    if (config.isMember("modulus") && config["modulus"].isUInt64()) {
        modulus = config["modulus"].asUInt64();
    }
}

bool RelabelConfig::Validate() {
    return true;
}


// Process returns a relabeled version of the given label set. The relabel configurations
// are applied in order of input.
// There are circumstances where Process will modify the input label.
// If you want to avoid issues with the input label set being modified, at the cost of
// higher memory usage, you can use lbls.Copy().
// If a label set is dropped, EmptyLabels and false is returned.
bool relabel::Process(const Labels& lbls, const std::vector<RelabelConfig>& cfgs, Labels& ret) {
    auto lb = LabelsBuilder();
    lb.Reset(lbls);
    if (!ProcessBuilder(lb, cfgs)) {
        ret = Labels();
        return false;
    }
    ret = lb.labels();
    return true;
}

// ProcessBuilder is like Process, but the caller passes a labels.Builder
// containing the initial set of labels, which is mutated by the rules.
bool relabel::ProcessBuilder(LabelsBuilder& lb, const std::vector<RelabelConfig>& cfgs) {
    for (const RelabelConfig& cfg : cfgs) {
        bool keep = Relabel(cfg, lb);
        if (!keep) {
            return false;
        }
    }
    return true;
}

bool relabel::Relabel(const RelabelConfig& cfg, LabelsBuilder& lb) {
    vector<string> values;
    for (auto item : cfg.sourceLabels) {
        values.push_back(lb.Get(item));
    }
    string val = boost::algorithm::join(values, cfg.separator);

    switch (cfg.action) {
        case Action::drop: {
            if (boost::regex_match(val, cfg.regex)) {
                return false;
            }
            break;
        }
        case Action::keep: {
            if (!boost::regex_match(val, cfg.regex)) {
                return false;
            }
            break;
        }
        case Action::dropequal: {
            if (lb.Get(cfg.targetLabel) == val) {
                return false;
            }
            break;
        }
        case Action::keepequal: {
            if (lb.Get(cfg.targetLabel) != val) {
                return false;
            }
            break;
        }
        case Action::replace: {
            bool indexes = boost::regex_search(val, cfg.regex);
            // If there is no match no replacement must take place.
            if (!indexes) {
                break;
            }
            // target := model.LabelName(cfg.Regex.ExpandString([]byte{}, cfg.TargetLabel, val, indexes))
            // boost::format_all标志会因此再次匹配，此处禁用
            LabelName target
                = LabelName(boost::regex_replace(val, cfg.regex, cfg.targetLabel, boost::format_first_only));
            if (!target.isValid()) {
                break;
            }
            // res := cfg.Regex.ExpandString([]byte{}, cfg.Replacement, val, indexes)
            string res = boost::regex_replace(val, cfg.regex, cfg.replacement, boost::format_first_only);
            if (res.size() == 0) {
                lb.DeleteLabel(target.labelName);
                break;
            }
            lb.Set(target.labelName, string(res));
            break;
        }
        case Action::lowercase: {
            lb.Set(cfg.targetLabel, boost::to_lower_copy(val));
            break;
        }
        case Action::uppercase: {
            lb.Set(cfg.targetLabel, boost::to_upper_copy(val));
            break;
        }
        case Action::hashmod: {
            uint8_t digest[MD5_DIGEST_LENGTH];
            MD5((uint8_t*)val.c_str(), val.length(), (uint8_t*)&digest);
            // Use only the last 8 bytes of the hash to give the same result as earlier versions of this code.
            uint64_t hashVal = 0;
            for (int i = 8; i < MD5_DIGEST_LENGTH; ++i) {
                hashVal = (hashVal << 8) | digest[i];
            }
            uint64_t mod = hashVal % cfg.modulus;
            lb.Set(cfg.targetLabel, to_string(mod));
            break;
        }
        // labelmap单测要验证
        case Action::labelmap: {
            lb.Range([&cfg, &lb](Label label) {
                if (boost::regex_match(label.name, cfg.regex)) {
                    // res := cfg.Regex.ReplaceAllString(l.Name, cfg.Replacement)
                    string res = boost::regex_replace(
                        label.name, cfg.regex, cfg.replacement, boost::match_default | boost::format_all);
                    lb.Set(res, label.value);
                }
            });
            break;
        }
        case Action::labeldrop: {
            lb.Range([&cfg, &lb](Label label) {
                if (boost::regex_match(label.name, cfg.regex)) {
                    lb.DeleteLabel(label.name);
                }
            });
            break;
        }
        case Action::labelkeep: {
            lb.Range([&cfg, &lb](Label label) {
                if (!boost::regex_match(label.name, cfg.regex)) {
                    lb.DeleteLabel(label.name);
                }
            });
            break;
        }
        default:
            // error
            printf("relabel: unknown relabel action type %s\n", ActionToString(cfg.action).c_str());
            break;
    }
    return true;
}
LabelName::LabelName() {
}
LabelName::LabelName(std::string labelName) : labelName(labelName) {
}

// UTF8Validation
bool LabelName::isValid() {
    return true;
}
} // namespace logtail
