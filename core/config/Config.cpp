// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Config.h"
#if defined(__linux__)
#include <fnmatch.h>
#endif
#include "common/Constants.h"
#include "common/FileSystemUtil.h"
#include "common/LogtailCommonFlags.h"
#include "reader/JsonLogFileReader.h"
#include "logger/Logger.h"

// DEFINE_FLAG_INT32(logreader_max_rotate_queue_size, "", 20);
DECLARE_FLAG_STRING(raw_log_tag);
DECLARE_FLAG_INT32(batch_send_interval);
DECLARE_FLAG_INT32(reader_close_unused_file_time);
DECLARE_FLAG_INT32(search_checkpoint_default_dir_depth);
DECLARE_FLAG_INT32(default_tail_limit_kb);
DECLARE_FLAG_INT32(logreader_max_rotate_queue_size);

namespace logtail {

Config::AdvancedConfig::AdvancedConfig()
    : mRawLogTag(STRING_FLAG(raw_log_tag))
//   mBatchSendInterval(INT32_FLAG(batch_send_interval)),
//   mMaxRotateQueueSize(INT32_FLAG(logreader_max_rotate_queue_size)),
//   mCloseUnusedReaderInterval(INT32_FLAG(reader_close_unused_file_time)),
//   mSearchCheckpointDirDepth(static_cast<uint16_t>(INT32_FLAG(search_checkpoint_default_dir_depth)))
{
}

Config::Config(const std::string& basePath,
               const std::string& filePattern,
              //  LogType logType,
               const std::string& logName,
               const std::string& logBeginReg,
               const std::string& logContinueReg,
               const std::string& logEndReg,
               const std::string& projectName,
               bool isPreserve,
               int preserveDepth,
               int maxDepth,
               const std::string& category,
               bool uploadRawLog /*= false*/,
               const std::string& StreamLogTag /*= ""*/,
               bool discardUnmatch /*= true*/,
               int readerFlushTimeout /*= 5*/,
               std::list<std::string>* regs /*= NULL*/,
               std::list<std::string>* keys /*= NULL*/,
               std::string timeFormat /* = ""*/)
    // : mBasePath(basePath),
    //   mFilePattern(filePattern),
      // : mLogType(logType),
      : mConfigName(logName),
    //   mLogBeginReg(logBeginReg),
    //   mLogContinueReg(logContinueReg),
    //   mLogEndReg(logEndReg),
    //   mReaderFlushTimeout(readerFlushTimeout),
    //   mProjectName(projectName),
    //   mIsPreserve(isPreserve),
    //   mPreserveDepth(preserveDepth),
    //   mMaxDepth(maxDepth),
      mRegs(regs),
      mKeys(keys),
      mTimeFormat(timeFormat),
    //   mCategory(category),
      mStreamLogTag(StreamLogTag),
      mDiscardUnmatch(discardUnmatch),
      mUploadRawLog(uploadRawLog) {
#if defined(_MSC_VER)
    mBasePath = EncodingConverter::GetInstance()->FromUTF8ToACP(mBasePath);
    mFilePattern = EncodingConverter::GetInstance()->FromUTF8ToACP(mFilePattern);
#endif

    // ParseWildcardPath();
    // mTailLimit = 0;
    // mTailExisted = false;
    // mLogstoreKey = GenerateLogstoreFeedBackKey(GetProjectName(), GetCategory());
    mSimpleLogFlag = false;
    // mCreateTime = 0;
    // mMaxSendBytesPerSecond = -1;
    // mSendRateExpireTime = -1;
    // mMergeType = MERGE_BY_TOPIC;
    mTimeZoneAdjust = false;
    mLogTimeZoneOffsetSecond = 0;
    // mLogDelayAlarmBytes = 0;
    // mPriority = 0;
    // mLogDelaySkipBytes = 0;
    // mLocalFlag = false;
    // mDockerFileFlag = false;
    // mPluginProcessFlag = false;
    mAcceptNoEnoughKeys = false;
}

// bool Config::IsMultiline() const {
//     return (!mLogBeginReg.empty() && mLogBeginReg != ".*") || (!mLogEndReg.empty() && mLogEndReg != ".*");
// }

} // namespace logtail
