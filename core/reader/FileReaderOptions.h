#pragma once

#include <cstdint>
#include <string>

#include "json/json.h"

#include "pipeline/PipelineContext.h"

namespace logtail {
struct FileReaderOptions {
    enum class Encoding { UTF8, GBK };

    Encoding mFileEncoding = Encoding::UTF8;
    bool mTailingAllMatchedFiles = false;
    uint32_t mTailSizeKB;
    uint32_t mFlushTimeoutSecs;
    uint32_t mReadDelaySkipThresholdBytes = 0;
    uint32_t mReadDelayAlertThresholdBytes;
    uint32_t mCloseUnusedReaderIntervalSec;
    uint32_t mRotatorQueueSize;

    FileReaderOptions();

    bool Init(const Json::Value& config, const PipelineContext& ctx, const std::string& pluginName);
};

} // namespace logtail
