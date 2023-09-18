#pragma once

#include <iostream>
#include <string>
#include "logclient/SlsClient.h"
#include "logger/Logger.h"
#include "pipeline/Error.h"
#include "pipeline/Pipeline.h"
#include "pipeline/Stats.h"
#include "plan/SplPlan.h"
#include "rw/IO.h"
#include "rw/IO.h"

namespace apsara::sls::spl {

bool getSplPlanFromParser(
    const std::string& splScript,
    std::string& resBody,
    Error& err);

SplPlanPtr getSplPlanPtr(
    const LoggerPtr& logger,
    const std::string& splScript,
    Error& err);

void executeFromBatchLog(
    const std::string& splPlan,
    const LoggerPtr& loggerPtr,
    SlsClient* slsClient,
    const std::string& project,
    const std::string& logstore,
    const int32_t batchCount,
    Error& err);

void executeFromPullLog(
    const std::string& splPlan,
    const LoggerPtr& loggerPtr,
    SlsClient* slsClient,
    const std::string& project,
    const std::string& logstore,
    const int32_t batchCount,
    Error& err,
    bool fromTest = false);

void executePipeline(
    const PipelinePtr spip,
    const LoggerPtr& loggerPtr,
    std::vector<InputPtr> rowReaders,
    Error& err);

void executeFromFile(
    const std::string& splPlan,
    const LoggerPtr& loggerPtr,
    const std::string& fileName,
    const std::string& batch,
    Error& err);

void executeCmd(
    std::string script,
    const std::string& mode,
    const LoggerPtr& loggerPtr,
    const std::string& fileName,
    const std::string& batch,
    SlsClient* slsClient,
    const std::string& project,
    const std::string& logstore,
    const int32_t batchCount,
    Error& err,
    bool fromTest = false);

}  // namespace apsara::sls::spl