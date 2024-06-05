// Copyright 2023 iLogtail Authors
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

#include "common/YamlUtil.h"
#include "common/ExceptionBase.h"
#include "common/StringTools.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using namespace std;

namespace logtail {

bool ParseYamlTable(const string& config, YAML::Node& yamlRoot, string& errorMsg) {
    try {
        yamlRoot = YAML::Load(config);
        if (CheckYamlCycle(yamlRoot)) {
            errorMsg = "yaml file contains cycle dependencies.";
            return false;
        }
    } catch (const YAML::ParserException& e) {
        errorMsg = "parse yaml failed: " + string(e.what());
        return false;
    } catch (const exception& e) {
        errorMsg = "unknown exception: " + string(e.what());
        return false;
    } catch (...) {
        errorMsg = "unknown error";
        return false;
    }
    return true;
}

bool VisitNode(const YAML::Node &node, std::vector<YAML::Node>& visited) {
    visited.push_back(node);
    if (node.IsMap()) {
        for (const auto &child : node) {
            if (std::find(visited.begin(), visited.end(), child.second) != visited.end()) {
                return true;  // Cycle detected
            }
            if (VisitNode(child.second, visited)) {
                return true;  // Propagate the failure up the call stack
            }
        }
    } else if (node.IsSequence()) {
        for (const auto &child : node) {
            if (std::find(visited.begin(), visited.end(), child) != visited.end()) {
                return true;  // Cycle detected
            }
            if (VisitNode(child, visited)) {
                return true;  // Propagate the failure up the call stack
            }
        }
    }
    // If the node is a scalar, we don't need to do anything special.
    visited.pop_back();
    return false;  // No cycle detected, continue recursion
}

bool CheckYamlCycle(const YAML::Node& root) {
    std::vector<YAML::Node> visited;
    return VisitNode(root, visited);
}

Json::Value ParseScalar(const YAML::Node& node) {
    // yaml-cpp automatically discards quotes in quoted values,
    // so to differentiate strings and integer for purely-digits value,
    // we can tell from node.Tag(), which will return "!" for quoted values and "?" for others
    if (node.Tag() == "!") {
        return node.as<string>();
    }

    int i;
    if (YAML::convert<int>::decode(node, i))
        return Json::Value(i);

    double d;
    if (YAML::convert<double>::decode(node, d))
        return Json::Value(d);

    bool b;
    if (YAML::convert<bool>::decode(node, b))
        return Json::Value(b);

    string s;
    if (YAML::convert<string>::decode(node, s))
        return Json::Value(s);

    return Json::Value();
}

Json::Value ConvertYamlToJson(const YAML::Node& rootNode) {
    Json::Value result;
    switch (rootNode.Type()) {
        case YAML::NodeType::Scalar:
            return ParseScalar(rootNode);
        case YAML::NodeType::Sequence: {
            result = Json::Value(Json::arrayValue); // create an empty array
            for (const auto& node : rootNode) {
                result.append(ConvertYamlToJson(node));
            }
            break;
        }
        case YAML::NodeType::Map: {
            result = Json::Value(Json::objectValue); // create an empty object
            for (const auto& node : rootNode) {
                result[node.first.as<string>()] = ConvertYamlToJson(node.second);
            }
            break;
        }
        default:
            break;
    }
    return result;
}

void EmitYamlWithQuotes(const YAML::Node& node, YAML::Emitter& out) {
    switch (node.Type()) {
        case YAML::NodeType::Null:
            out << YAML::Null;
            break;
        case YAML::NodeType::Scalar:
            {
                std::string value = node.Scalar();
                if (node.Tag() == "!") { // check ""
                    out << YAML::DoubleQuoted << value;
                } else {
                    out << value;
                }
            }
            break;
        case YAML::NodeType::Sequence:
            out << YAML::BeginSeq;
            for (const auto& subNode : node) {
                EmitYamlWithQuotes(subNode, out);
            }
            out << YAML::EndSeq;
            break;
        case YAML::NodeType::Map:
            out << YAML::BeginMap;
            for (const auto& it : node) {
                out << YAML::Key;
                EmitYamlWithQuotes(it.first, out);
                out << YAML::Value;
                EmitYamlWithQuotes(it.second, out);
            }
            out << YAML::EndMap;
            break;
    }
}

bool UpdateLegacyConfigYaml(YAML::Node& yamlContent, string& errorMsg) {
    // Check if 'version' field exists, then update it to 'global.StructureType'
    if (yamlContent["version"]) {
        yamlContent["global"]["StructureType"] = yamlContent["version"];
        yamlContent.remove("version"); // Remove the old 'version' field
    }

    // rename "file_log" to "input_file"
    // combine LogPath and FilePattern to FilePaths
    if (yamlContent["inputs"]) {
        YAML::Node inputsNode = yamlContent["inputs"];
        for (std::size_t i = 0; i < inputsNode.size(); ++i) {
            YAML::Node input = inputsNode[i]; // Make a copy of the YAML::Node object
            if (input["Type"] && input["Type"].as<string>() == "file_log") {
                // Change type file_log to input_file
                input["Type"] = "input_file";
                
                // Update the 'FilePaths' field
                if (input["LogPath"] && input["FilePattern"]) {
                    string dirPath = input["LogPath"].as<string>();
                    string filePattern =  input["FilePattern"].as<string>();

                    filesystem::path filePath = dirPath;
                    // Append directory search depth if MaxDepth is specified
                    if (input["MaxDepth"] && input["MaxDepth"].as<int>() > 0) {
                        filePath /= "**";
                        input["MaxDirSearchDepth"] = input["MaxDepth"];
                    }

                    filePath /= filePattern;
                    string finalPath = filePath.string();
                    input["FilePaths"] = std::vector<std::string>{finalPath};
                } else {
                    errorMsg = "LogPath or FilePattern not found in file_log input";
                    return false;
                }
                
                // delete LogPath, FilePattern, and MaxDepth
                input.remove("LogPath");
                input.remove("FilePattern");
                input.remove("MaxDepth");

                if (input["TopicFormat"]) {
                    string topicFormat = input["TopicFormat"].as<string>();
                    string prefix = "costomized://";
                    if (StartWith(topicFormat, prefix)) {
                        // remove "customized://"
                        input["TopicFormat"] = topicFormat.substr(prefix.size());
                        yamlContent["global"]["TopicType"] = "customized";                        
                    }
                }
            }
            inputsNode[i] = input;            
        }
        // Update the 'inputs' section in the YAML content
        yamlContent["inputs"] = inputsNode;
    }


    // rename processor_regex_accelerate to processor_parse_regex_native
    // rename processor_json_accelerate to processor_parse_json_native
    // rename processor_delimiter_accelerate to processor_parse_delimiter_native
    if (yamlContent["processors"]) {
        YAML::Node processorsNode = yamlContent["processors"];
        for (std::size_t i = 0; i < processorsNode.size(); ++i) {
            YAML::Node processor = processorsNode[i];
            if (processor["Type"] && processor["Type"].as<string>() == "processor_regex_accelerate") {
                processor["Type"] = "processor_parse_regex_native";
            } else if (processor["Type"] && processor["Type"].as<string>() == "processor_json_accelerate") {
                processor["Type"] = "processor_parse_json_native";
            } else if (processor["Type"] && processor["Type"].as<string>() == "processor_delimiter_accelerate") {
                processor["Type"] = "processor_parse_delimiter_native";
            }
            processorsNode[i] = processor;
        }
        // Update the 'processors' section in the YAML content
        yamlContent["processors"] = processorsNode;
    }

    return true;
}

} // namespace logtail
