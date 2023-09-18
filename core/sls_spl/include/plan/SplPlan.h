#pragma once

#include <iostream>
#include <memory>
#include <json.hpp>
#include <unordered_map>
#include <vector>

namespace apsara::sls::spl {

struct SplSourceLocation {
    int32_t line_;
    int32_t column_;
};

using SplSourceLocationPtr = std::shared_ptr<SplSourceLocation>;
void from_json(const nlohmann::json& j, SplSourceLocation& s);

struct SplNodePlan {
    std::string content_;
    std::string compressType_;
    std::unordered_map<std::string, std::string> options_;
    std::vector<std::vector<std::string>> expressions_;
    std::vector<std::string> outputs_;
    std::vector<std::string> fallbacks_;
    bool isAlwaysFalse();
    bool isAlwaysTrue();
};
using SplNodePlanPtr = std::shared_ptr<SplNodePlan>;
void from_json(const nlohmann::json& j, SplNodePlan& s);

enum class SplNodeType {
    PRINT,
    WHERE,
    EXTEND,
    PROJECT,
    PROJECT_AWAY,
    PROJECT_RENAME,
    EXPAND_VALUES,
    PARSE_REGEXP,
    PARSE_JSON,
    PARSE_CSV,
    PARSE_KV,
    REFERENCE,
    UNKNOWN
};

std::string to_string(const SplNodeType& n);

SplNodeType from_string(const std::string& s);

std::ostream& operator<<(std::ostream& os, const SplNodeType& s);

struct SplNode {
    SplNodeType type_;
    std::string query_;
    SplNodePlanPtr plan_;
    SplSourceLocationPtr sourceLocation_;
    std::vector<uint32_t> successors_;
    std::string label_;
    std::string errWithLocation(const std::string& errMsg);
};
using SplNodePtr = std::shared_ptr<SplNode>;
void from_json(const nlohmann::json& j, SplNode& s);
struct SplPlanSource {
    std::vector<std::string> search_;
    std::vector<std::vector<uint32_t>> mapping_;
};

using SplPlanSourcePtr = std::shared_ptr<SplPlanSource>;
void from_json(const nlohmann::json& j, SplPlanSource& s);
struct SplPlan {
    std::string error_;
    std::unordered_map<std::string, std::string> settings_;
    SplPlanSourcePtr sources_;
    std::vector<SplNodePtr> nodes_;
};

using SplPlanPtr = std::shared_ptr<SplPlan>;
void from_json(const nlohmann::json& j, SplPlan& s);

}  // namespace apsara::sls::spl