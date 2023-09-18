#pragma once

#include <iostream>
#include <presto_cpp/external/json/json.hpp>
#include <unordered_map>
#include "logger/Logger.h"
#include "pipeline/Error.h"
#include "plan/SplPlan.h"
#include "util/Type.h"

namespace apsara::sls::spl {

SplPlanPtr parseSplPlan(
    const LoggerPtr& logger,
    const std::string& plan,
    Error& error);

void parseNodePlan(
    const LoggerPtr& logger,
    const std::string& plan,
    nlohmann::json& planJson,
    std::unordered_map<std::string, std::string>& colRenameMap,
    Error& error);

velox::core::PlanNodePtr planToNode(
    const LoggerPtr& logger,
    const MemoryPoolPtr& allocatePool,
    const SplNodeType& planType,
    const std::string& plan,
    std::unordered_map<std::string, std::string>& colRenameMap,
    Error& error);

FilterNodePtr planToFilterNode(
    const LoggerPtr& logger,
    const MemoryPoolPtr& allocatePool,
    const std::string& plan,
    std::unordered_map<std::string, std::string>& colMap,
    Error& error);

ProjectNodePtr planToProjectNode(
    const LoggerPtr& logger,
    const MemoryPoolPtr& allocatePool,
    const std::string& plan,
    std::unordered_map<std::string, std::string>& colMap,
    Error& error);

}  // namespace apsara::sls::spl