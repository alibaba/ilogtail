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

#include "LogFileReader.h"
#include <time.h>
#include <limits>
#include <numeric>
#if defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#endif
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <cityhash/city.h>
#include "common/util.h"
#include "common/Flags.h"
#include "common/HashUtil.h"
#include "common/ErrorUtil.h"
#include "common/TimeUtil.h"
#include "common/FileSystemUtil.h"
#include "common/RandomUtil.h"
#include "common/Constants.h"
#include "sdk/Common.h"
#include "logger/Logger.h"
#include "checkpoint/CheckPointManager.h"
#include "checkpoint/CheckpointManagerV2.h"
#include "monitor/LogtailAlarm.h"
#include "monitor/LogFileProfiler.h"
#include "event_handler/LogInput.h"
#include "app_config/AppConfig.h"
#include "config_manager/ConfigManager.h"
#include "common/LogFileCollectOffsetIndicator.h"
#include "fuse/UlogfsHandler.h"
#include "sender/Sender.h"
#include "GloablFileDescriptorManager.h"
#include "event/BlockEventManager.h"

using namespace sls_logs;
using namespace std;

DEFINE_FLAG_INT32(delay_bytes_upperlimit,
                  "if (total_file_size - current_readed_size) exceed uppperlimit, send READ_LOG_DELAY_ALARM, bytes",
                  200 * 1024 * 1024);
DEFINE_FLAG_INT32(read_delay_alarm_duration,
                  "if read delay elapsed this duration, send READ_LOG_DELAY_ALARM, seconds",
                  60);
DEFINE_FLAG_INT32(reader_close_unused_file_time, "second ", 60);
DEFINE_FLAG_INT32(skip_first_modify_time, "second ", 5 * 60);
DEFINE_FLAG_INT32(max_reader_open_files, "max fd count that reader can open max", 100000);
DEFINE_FLAG_INT32(truncate_pos_skip_bytes, "skip more xx bytes when truncate", 0);
DEFINE_FLAG_INT32(max_fix_pos_bytes, "", 128 * 1024);
DEFINE_FLAG_INT32(force_release_deleted_file_fd_timeout,
                  "force release fd if file is deleted after specified seconds, no matter read to end or not",
                  -1);

namespace logtail {

#define COMMON_READER_INFO \
    ("project", mProjectName)("logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)( \
        "file device", mDevInode.dev)("file inode", mDevInode.inode)("file signature", mLastFileSignatureHash)

size_t LogFileReader::BUFFER_SIZE = 1024 * 512; // 512KB

void LogFileReader::DumpMetaToMem(bool checkConfigFlag) {
    if (checkConfigFlag) {
        size_t index = mHostLogPath.rfind(PATH_SEPARATOR);
        if (index == string::npos || index == mHostLogPath.size() - 1) {
            LOG_INFO(sLogger,
                     ("skip dump reader meta", "invalid log reader queue name")("project", mProjectName)(
                         "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                         "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                         "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize));
            return;
        }
        string dirPath = mHostLogPath.substr(0, index);
        string fileName = mHostLogPath.substr(index + 1, mHostLogPath.size() - index - 1);
        if (ConfigManager::GetInstance()->FindBestMatch(dirPath, fileName) == NULL) {
            LOG_INFO(sLogger,
                     ("skip dump reader meta", "no config matches the file path")("project", mProjectName)(
                         "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                         "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                         "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize));
            return;
        }
        LOG_INFO(sLogger,
                 ("dump log reader meta, project", mProjectName)("logstore", mCategory)("config", mConfigName)(
                     "log reader queue name", mHostLogPath)("file device", ToString(mDevInode.dev))(
                     "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                     "file signature size", mLastFileSignatureSize)("real file path", mRealLogPath)(
                     "file size", mLastFileSize)("last file position", mLastFilePos)("is file opened",
                                                                                     ToString(mLogFileOp.IsOpen())));
    }
    CheckPoint* checkPointPtr = new CheckPoint(mHostLogPath,
                                               mLastFilePos,
                                               mLastFileSignatureSize,
                                               mLastFileSignatureHash,
                                               mDevInode,
                                               mConfigName,
                                               mRealLogPath,
                                               mLogFileOp.IsOpen(),
                                               mContainerStopped,
                                               mLastForceRead);
    // use last event time as checkpoint's last update time
    checkPointPtr->mLastUpdateTime = mLastEventTime;
    checkPointPtr->mCache = mCache;
    CheckPointManager::Instance()->AddCheckPoint(checkPointPtr);
}

void LogFileReader::SetFileDeleted(bool flag) {
    mFileDeleted = flag;
    if (flag) {
        mDeletedTime = time(NULL);
    }
}

void LogFileReader::SetContainerStopped() {
    if (!mContainerStopped) {
        mContainerStopped = true;
        mContainerStoppedTime = time(NULL);
    }
}

bool LogFileReader::ShouldForceReleaseDeletedFileFd() {
    time_t now = time(NULL);
    return INT32_FLAG(force_release_deleted_file_fd_timeout) >= 0
        && ((IsFileDeleted() && now - GetDeletedTime() >= INT32_FLAG(force_release_deleted_file_fd_timeout))
            || (IsContainerStopped()
                && now - GetContainerStoppedTime() >= INT32_FLAG(force_release_deleted_file_fd_timeout)));
}

void LogFileReader::InitReader(bool tailExisted, FileReadPolicy policy, uint32_t eoConcurrency) {
    string buffer = LogFileProfiler::mIpAddr + "_" + mHostLogPath + "_" + CalculateRandomUUID();
    uint64_t cityHash = CityHash64(buffer.c_str(), buffer.size());
    mSourceId = ToHexString(cityHash);

    if (!tailExisted) {
        static CheckPointManager* checkPointManagerPtr = CheckPointManager::Instance();
        // hold on checkPoint share ptr, so this check point will not be delete in this block
        CheckPointPtr checkPointSharePtr;
        if (checkPointManagerPtr->GetCheckPoint(mDevInode, mConfigName, checkPointSharePtr)) {
            CheckPoint* checkPointPtr = checkPointSharePtr.get();
            mLastFilePos = checkPointPtr->mOffset;
            mLastForceRead = checkPointPtr->mLastForceRead;
            mCache = checkPointPtr->mCache;
            mLastFileSignatureHash = checkPointPtr->mSignatureHash;
            mLastFileSignatureSize = checkPointPtr->mSignatureSize;
            mRealLogPath = checkPointPtr->mRealFileName;
            mLastEventTime = checkPointPtr->mLastUpdateTime;
            mContainerStopped = checkPointPtr->mContainerStopped;
            LOG_INFO(sLogger,
                     ("recover log reader status from checkpoint, project", mProjectName)("logstore", mCategory)(
                         "config", mConfigName)("log reader queue name", mHostLogPath)(
                         "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                         "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                         "real file path", mRealLogPath)("last file position", mLastFilePos));
            // if file is open or
            // last update time is new and the file's container is not stopped we
            // we should use first modify
            if (checkPointPtr->mFileOpenFlag
                || ((int32_t)time(NULL) - checkPointPtr->mLastUpdateTime < INT32_FLAG(skip_first_modify_time)
                    && !mContainerStopped)) {
                mSkipFirstModify = false;
            } else {
                mSkipFirstModify = true;
            }
            // delete checkpoint at last
            checkPointManagerPtr->DeleteCheckPoint(mDevInode, mConfigName);
            // because the reader is initialized by checkpoint, so set first watch to false
            mFirstWatched = false;
        }
    }

    if (eoConcurrency > 0) {
        initExactlyOnce(eoConcurrency);
    }

    if (mFirstWatched) {
        CheckForFirstOpen(policy);
    }
}

namespace detail {

    void updatePrimaryCheckpoint(const std::string& key, PrimaryCheckpointPB& cpt, const std::string& field) {
        cpt.set_update_time(time(NULL));
        if (CheckpointManagerV2::GetInstance()->SetPB(key, cpt)) {
            LOG_INFO(sLogger, ("update primary checkpoint", key)("field", field)("checkpoint", cpt.DebugString()));
        } else {
            LOG_WARNING(sLogger,
                        ("update primary checkpoint error", key)("field", field)("checkpoint", cpt.DebugString()));
        }
    }

    std::pair<size_t, size_t> getPartitionRange(size_t idx, size_t concurrency, size_t totalPartitionCount) {
        auto base = totalPartitionCount / concurrency;
        auto extra = totalPartitionCount % concurrency;
        if (extra == 0) {
            return std::make_pair(idx * base, (idx + 1) * base - 1);
        }
        size_t min = idx <= extra ? idx * (base + 1) : extra * (base + 1) + (idx - extra) * base;
        size_t max = idx < extra ? min + base : min + base - 1;
        return std::make_pair(min, max);
    }

} // namespace detail

void LogFileReader::initExactlyOnce(uint32_t concurrency) {
    mEOOption.reset(new ExactlyOnceOption);

    // Primary key.
    auto& primaryCptKey = mEOOption->primaryCheckpointKey;
    primaryCptKey = makePrimaryCheckpointKey();

    // Recover primary checkpoint from local if have.
    PrimaryCheckpointPB primaryCpt;
    static auto sCptM = CheckpointManagerV2::GetInstance();
    bool hasCheckpoint = sCptM->GetPB(primaryCptKey, primaryCpt);
    if (hasCheckpoint) {
        hasCheckpoint = validatePrimaryCheckpoint(primaryCpt);
        if (!hasCheckpoint) {
            LOG_WARNING(sLogger,
                        COMMON_READER_INFO("ignore primary checkpoint and delete range checkpoints", primaryCptKey));
            std::vector<std::string> rangeCptKeys;
            CheckpointManagerV2::AppendRangeKeys(primaryCptKey, concurrency, rangeCptKeys);
            sCptM->DeleteCheckpoints(rangeCptKeys);
        }
    }
    mEOOption->concurrency = hasCheckpoint ? primaryCpt.concurrency() : concurrency;
    if (!hasCheckpoint) {
        primaryCpt.set_concurrency(mEOOption->concurrency);
        primaryCpt.set_sig_hash(mLastFileSignatureHash);
        primaryCpt.set_sig_size(mLastFileSignatureSize);
        primaryCpt.set_config_name(mConfigName);
        primaryCpt.set_log_path(mHostLogPath);
        primaryCpt.set_real_path(mRealLogPath.empty() ? mHostLogPath : mRealLogPath);
        primaryCpt.set_dev(mDevInode.dev);
        primaryCpt.set_inode(mDevInode.inode);
        detail::updatePrimaryCheckpoint(mEOOption->primaryCheckpointKey, primaryCpt, "all (new)");
    }
    mEOOption->primaryCheckpoint.Swap(&primaryCpt);
    LOG_INFO(sLogger,
             ("primary checkpoint", primaryCptKey)("old", hasCheckpoint)("checkpoint",
                                                                         mEOOption->primaryCheckpoint.DebugString()));

    // Randomize range checkpoints index for load balance.
    std::vector<uint32_t> concurrencySequence(mEOOption->concurrency);
    std::iota(concurrencySequence.begin(), concurrencySequence.end(), 0);
    std::random_shuffle(concurrencySequence.begin(), concurrencySequence.end());
    // Initialize range checkpoints (recover from local if have).
    mEOOption->rangeCheckpointPtrs.resize(mEOOption->concurrency);
    std::string baseHashKey;
    for (size_t idx = 0; idx < concurrencySequence.size(); ++idx) {
        const uint32_t partIdx = concurrencySequence[idx];
        auto& rangeCpt = mEOOption->rangeCheckpointPtrs[idx];
        rangeCpt.reset(new RangeCheckpoint);
        rangeCpt->index = idx;
        rangeCpt->key = CheckpointManagerV2::MakeRangeKey(mEOOption->primaryCheckpointKey, partIdx);

        // No checkpoint, generate random hash key.
        bool newCpt = !hasCheckpoint || !sCptM->GetPB(rangeCpt->key, rangeCpt->data);
        if (newCpt) {
            if (baseHashKey.empty()) {
                baseHashKey = GenerateRandomHashKey();
            }

            // Map to partition range so that it can fits the case that the number of
            //  logstore's shards is bigger than concurreny.
            const size_t kPartitionCount = 512;
            auto partitionRange = detail::getPartitionRange(partIdx, mEOOption->concurrency, kPartitionCount);
            auto partitionID = partitionRange.first + rand() % (partitionRange.second - partitionRange.first + 1);
            rangeCpt->data.set_hash_key(GenerateHashKey(baseHashKey, partitionID, kPartitionCount));
            rangeCpt->data.set_sequence_id(sdk::kFirstHashKeySeqID);
            rangeCpt->data.set_committed(false);
        }
        LOG_DEBUG(sLogger,
                  ("range checkpoint", rangeCpt->key)("index", idx)("new", newCpt)("checkpoint",
                                                                                   rangeCpt->data.DebugString()));
    }

    // Initialize feedback queues.
    mEOOption->fbKey = QueueManager::GetInstance()->InitializeExactlyOnceQueues(
        mProjectName,
        mEOOption->primaryCheckpointKey + mEOOption->rangeCheckpointPtrs[0]->data.hash_key(),
        mEOOption->rangeCheckpointPtrs);
    for (auto& cpt : mEOOption->rangeCheckpointPtrs) {
        cpt->fbKey = mEOOption->fbKey;
    }

    adjustParametersByRangeCheckpoints();
}

bool LogFileReader::validatePrimaryCheckpoint(const PrimaryCheckpointPB& cpt) {
#define METHOD_LOG_PATTERN ("method", "validatePrimaryCheckpoint")("checkpoint", cpt.DebugString())
    auto const sigSize = cpt.sig_size();
    auto const sigHash = cpt.sig_hash();
    if (sigSize > 0) {
        auto filePath = cpt.real_path().empty() ? cpt.log_path() : cpt.real_path();
        auto hasFileBeenRotated = [&]() {
            auto devInode = GetFileDevInode(filePath);
            if (devInode == mDevInode) {
                return false;
            }

            auto dirPath = boost::filesystem::path(filePath).parent_path();
            const auto searchResult = SearchFilePathByDevInodeInDirectory(dirPath.string(), 0, mDevInode, nullptr);
            if (!searchResult) {
                LOG_WARNING(sLogger, METHOD_LOG_PATTERN("can not find file with dev inode", mDevInode.inode));
                return false;
            }
            const auto& newFilePath = searchResult.value();
            LOG_INFO(sLogger,
                     METHOD_LOG_PATTERN("file has been rotated from", "")("from", filePath)("to", newFilePath));
            filePath = newFilePath;
            return true;
        };
        if (CheckFileSignature(filePath, sigHash, sigSize, mIsFuseMode)
            || (hasFileBeenRotated() && CheckFileSignature(filePath, sigHash, sigSize, mIsFuseMode))) {
            mLastFileSignatureSize = sigSize;
            mLastFileSignatureHash = sigHash;
            mRealLogPath = filePath;
            return true;
        }
        LOG_WARNING(sLogger, METHOD_LOG_PATTERN("mismatch with local file content", filePath));
        return false;
    }
    if (mLastFileSignatureSize > 0) {
        LOG_WARNING(
            sLogger,
            METHOD_LOG_PATTERN("mismatch with checkpoint v1 signature, sig size", sigSize)("sig hash", sigHash));
        return false;
    }
    return false;
#undef METHOD_LOG_PATTERN
}

void LogFileReader::adjustParametersByRangeCheckpoints() {
    auto& uncommittedCheckpoints = mEOOption->toReplayCheckpoints;
    uint32_t maxOffsetIndex = mEOOption->concurrency;
    for (uint32_t idx = 0; idx < mEOOption->concurrency; ++idx) {
        auto& rangeCpt = mEOOption->rangeCheckpointPtrs[idx];
        if (!rangeCpt->data.has_read_offset()) {
            continue;
        } // Skip new checkpoint.

        if (!rangeCpt->data.committed()) {
            uncommittedCheckpoints.push_back(rangeCpt);
        } else {
            rangeCpt->IncreaseSequenceID();
        }

        const int64_t maxReadOffset = maxOffsetIndex != mEOOption->concurrency
            ? mEOOption->rangeCheckpointPtrs[maxOffsetIndex]->data.read_offset()
            : -1;
        if (static_cast<int64_t>(rangeCpt->data.read_offset()) > maxReadOffset) {
            maxOffsetIndex = idx;
        }
    }

    // Find uncommitted checkpoints, sort them by offset for replay.
    if (!uncommittedCheckpoints.empty()) {
        std::sort(uncommittedCheckpoints.begin(),
                  uncommittedCheckpoints.end(),
                  [](const RangeCheckpointPtr& lhs, const RangeCheckpointPtr& rhs) {
                      return lhs->data.read_offset() < rhs->data.read_offset();
                  });
        auto& firstCpt = uncommittedCheckpoints.front()->data;
        mLastFilePos = firstCpt.read_offset();
        mCache.clear();
        mFirstWatched = false;

        // Set skip position if there are comitted checkpoints.
        if (maxOffsetIndex != mEOOption->concurrency) {
            auto& cpt = mEOOption->rangeCheckpointPtrs[maxOffsetIndex]->data;
            if (cpt.committed()) {
                mEOOption->lastComittedOffset = static_cast<int64_t>(cpt.read_offset() + cpt.read_length());
            }
        }

        LOG_INFO(
            sLogger,
            ("initialize reader", "uncommitted checkpoints")COMMON_READER_INFO("count", uncommittedCheckpoints.size())(
                "first checkpoint", firstCpt.DebugString())("last committed offset", mEOOption->lastComittedOffset));
    }
    // All checkpoints are committed, skip them.
    else if (maxOffsetIndex != mEOOption->concurrency) {
        auto& cpt = mEOOption->rangeCheckpointPtrs[maxOffsetIndex]->data;
        mLastFilePos = cpt.read_offset() + cpt.read_length();
        mCache.clear();
        mFirstWatched = false;
        LOG_INFO(sLogger,
                 ("initialize reader", "checkpoint with max offset")COMMON_READER_INFO("max index", maxOffsetIndex)(
                     "checkpoint", cpt.DebugString()));
    } else {
        LOG_INFO(sLogger,
                 ("initialize reader", "no available checkpoint")COMMON_READER_INFO("first watch", mFirstWatched));
    }
}

void LogFileReader::updatePrimaryCheckpointSignature() {
    auto& cpt = mEOOption->primaryCheckpoint;
    cpt.set_sig_size(mLastFileSignatureSize);
    cpt.set_sig_hash(mLastFileSignatureHash);
    detail::updatePrimaryCheckpoint(mEOOption->primaryCheckpointKey, cpt, "signature");
}

void LogFileReader::updatePrimaryCheckpointRealPath() {
    auto& cpt = mEOOption->primaryCheckpoint;
    cpt.set_real_path(mRealLogPath);
    detail::updatePrimaryCheckpoint(mEOOption->primaryCheckpointKey, cpt, "real_path");
}

LogFileReader::LogFileReader(const string& projectName,
                             const string& category,
                             const string& hostLogPathDir,
                             const std::string& hostLogPathFile,
                             int32_t tailLimit,
                             bool discardUnmatch,
                             bool dockerFileFlag) {
    mFirstWatched = true;
    mProjectName = projectName;
    mCategory = category;
    mTopicName = "";
    mHostLogPathDir = hostLogPathDir;
    mHostLogPathFile = hostLogPathFile;
    mHostLogPath = PathJoin(hostLogPathDir, hostLogPathFile);
    // mRealLogPath = mHostLogPath; fix it in 1.8
    mTailLimit = tailLimit;
    mLastFilePos = 0;
    mLastFileSize = 0;
    mLogBeginRegPtr = NULL;
    mLogContinueRegPtr = NULL;
    mLogEndRegPtr = NULL;
    mReaderFlushTimeout = 5;
    mLastForceRead = false;
    mDiscardUnmatch = discardUnmatch;
    mLastUpdateTime = time(NULL);
    mLastEventTime = mLastUpdateTime;
    mFileDeleted = false;
    mDeletedTime = (time_t)0;
    mContainerStopped = false;
    mContainerStoppedTime = (time_t)0;
    mReadStoppedContainerAlarmTime = (time_t)0;
    mLastFileSignatureHash = 0;
    mLastFileSignatureSize = 0;
    mReadDelayTime = 0;
    mReaderArray = NULL;
    mSkipFirstModify = false;
    mReadDelayAlarmBytes = INT32_FLAG(delay_bytes_upperlimit);
    mPluginFlag = false;
    mPackId = 0;
    mReadDelaySkipBytes = 0;
    mCloseUnusedInterval = INT32_FLAG(reader_close_unused_file_time);
    mPreciseTimestampConfig.enabled = false;
}

LogFileReader::LogFileReader(const std::string& projectName,
                             const std::string& category,
                             const std::string& hostLogPathDir,
                             const std::string& hostLogPathFile,
                             int32_t tailLimit,
                             const std::string& topicFormat,
                             const std::string& groupTopic,
                             FileEncoding fileEncoding,
                             bool discardUnmatch,
                             bool dockerFileFlag) {
    mFirstWatched = true;
    mProjectName = projectName;
    mCategory = category;
    mHostLogPathDir = hostLogPathDir;
    mHostLogPathFile = hostLogPathFile;
    mHostLogPath = PathJoin(hostLogPathDir, hostLogPathFile);
    // mRealLogPath = mHostLogPath; fix it in 1.8
    mTailLimit = tailLimit;
    mLastFilePos = 0;
    mLastFileSize = 0;
    const std::string lowerConfig = ToLowerCaseString(topicFormat);
    if (lowerConfig == "none" || lowerConfig == "default" || lowerConfig == "customized") {
        // For customized, it will be set through SetTopicName.
        mTopicName = "";
    } else if (lowerConfig == "global_topic") {
        static LogtailGlobalPara* sGlobalPara = LogtailGlobalPara::Instance();
        mTopicName = sGlobalPara->GetTopic();
    } else if (lowerConfig == "group_topic")
        mTopicName = groupTopic;
    else if (!dockerFileFlag) // if docker file, wait for reset topic format
        mTopicName = GetTopicName(topicFormat, mHostLogPath);
    mFileEncoding = fileEncoding;
    mLogBeginRegPtr = NULL;
    mLogContinueRegPtr = NULL;
    mLogEndRegPtr = NULL;
    mReaderFlushTimeout = 5;
    mLastForceRead = false;
    mDiscardUnmatch = discardUnmatch;
    mLastUpdateTime = time(NULL);
    mLastEventTime = mLastUpdateTime;
    mFileDeleted = false;
    mDeletedTime = (time_t)0;
    mContainerStopped = false;
    mContainerStoppedTime = (time_t)0;
    mReadStoppedContainerAlarmTime = (time_t)0;
    mLastFileSignatureHash = 0;
    mLastFileSignatureSize = 0;
    mReadDelayTime = 0;
    mReaderArray = NULL;
    mSkipFirstModify = false;
    mReadDelayAlarmBytes = INT32_FLAG(delay_bytes_upperlimit);
    mPluginFlag = false;
    mPackId = 0;
    mReadDelaySkipBytes = 0;
    mCloseUnusedInterval = INT32_FLAG(reader_close_unused_file_time);
    mPreciseTimestampConfig.enabled = false;
}

void LogFileReader::SetDockerPath(const std::string& dockerBasePath, size_t dockerReplaceSize) {
    if (dockerReplaceSize > (size_t)0 && mHostLogPath.size() > dockerReplaceSize && !dockerBasePath.empty()) {
        if (dockerBasePath.size() == (size_t)1) {
            mDockerPath = mHostLogPath.substr(dockerReplaceSize);
        } else {
            mDockerPath = dockerBasePath + mHostLogPath.substr(dockerReplaceSize);
        }

        LOG_DEBUG(sLogger, ("convert docker file path", "")("host path", mHostLogPath)("docker path", mDockerPath));
    }
}

void LogFileReader::SetReadFromBeginning() {
    mLastFilePos = 0;
    mCache.clear();
    LOG_INFO(sLogger,
             ("force reading file from the beginning, project", mProjectName)("logstore", mCategory)(
                 "config", mConfigName)("log reader queue name", mHostLogPath)("file device", ToString(mDevInode.dev))(
                 "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                 "file signature size", mLastFileSignatureSize)("file size", mLastFileSize));
    mFirstWatched = false;
}

bool LogFileReader::SetReadPosForBackwardReading(LogFileOperator& op) {
    if (mTimeFormat.empty()) {
        LOG_ERROR(sLogger, ("time format is empty", "cannnot set read pos backward"));
        return false;
    }

    int32_t systemBootTime = LogFileProfiler::mSystemBootTime;
    if (systemBootTime <= 0) {
        LOG_ERROR(sLogger, ("invalid boot time", "cannnot set read pos backward"));
        return false;
    }

    int64_t fileSize = op.GetFileSize();
    int64_t begin = 0, end = fileSize - 1, nextPos = -1;

    bool found = false;
    int32_t logTime = -1;
    while (end - begin > 0) {
        logTime = ParseTimeInBuffer(op, begin, end, systemBootTime, mTimeFormat, nextPos, found);
        if (logTime == -1) {
            // if we cannot parse time in this buffer, break
            LOG_WARNING(sLogger, ("failed to parse time", "cannnot set read pos backward"));
            return false;
        }

        if (found)
            break;

        if (logTime < systemBootTime)
            begin = nextPos;
        else // if (logTime >= systemBootTime)
            end = nextPos;
    }

    if (found || nextPos != -1) {
        mLastFilePos = nextPos;
        mCache.clear();
    }
    return true;
}

int32_t LogFileReader::ParseTimeInBuffer(LogFileOperator& op,
                                         int64_t begin,
                                         int64_t end,
                                         int32_t bootTime,
                                         const std::string& timeFormat,
                                         int64_t& filePos,
                                         bool& found) {
    if (begin >= end)
        return -1;

    if (!op.IsOpen()) {
        return -1;
    }

    // we should fix begin and end
    int64_t mid = (begin + end) / 2;
    begin = std::max(begin, (int64_t)(mid - BUFFER_SIZE));
    end = std::min((int64_t)(mid + BUFFER_SIZE), end);
    size_t size = (size_t)(end - begin) + 1;

    char* buffer = new char[size]();
    size_t nbytes = ReadFile(op, buffer, size, begin);

    // find start pos and end pos, searching for '\n'
    int lineBegin = 0, lineEnd = (int)nbytes - 1;
    // if begin is 0, we should not find \n from begin
    while (begin != 0 && lineBegin < (int)nbytes - 1) {
        if (buffer[lineBegin++] == '\n')
            break;
    }
    while (lineEnd > 0) {
        if (buffer[lineEnd] == '\n')
            break;
        --lineEnd;
    }

    if (lineBegin > lineEnd) {
        delete[] buffer;
        return -1;
    }

    // now lineBegin is the beginning of first whole line
    // and lineEnd is the end of last whole, specially, buffer[lineEnd] is \n
    // so sz is the size of whole lines
    size_t sz = (size_t)(lineEnd - lineBegin) + 1;
    int32_t parsedTime = -1, pos = -1;
    int result = ParseAllLines(buffer + lineBegin, sz, bootTime, timeFormat, parsedTime, pos);

    if (result == 1)
        filePos = begin + lineBegin + pos;
    else if (result == 2)
        filePos = begin + lineEnd + 1;
    else {
        if (result == 0) {
            found = true;
            filePos = begin + lineBegin + pos;
        }
        // if result == -1, parsedTime is -1
    }

    delete[] buffer;
    return parsedTime;
}

int LogFileReader::ParseAllLines(
    char* buffer, size_t size, int32_t bootTime, const std::string& timeFormat, int32_t& parsedTime, int& pos) {
    std::vector<int> lineFeedPos;
    int begin = 0;
    // here we push back 0 because the pos 0 must be the beginning of the first line
    lineFeedPos.push_back(begin);

    // data in buffer is between [0, size)
    for (int i = 1; i < (int)size; ++i) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            begin = i + 1;
            // if begin == size, we meet the end of all lines buffer
            if (begin < (int)size)
                lineFeedPos.push_back(begin);
        }
    }

    // parse first and last line
    size_t firstLogIndex = 0, lastLogIndex = lineFeedPos.size() - 1;
    int32_t firstLogTime = -1, lastLogTime = -1;
    for (size_t i = 0; i < lineFeedPos.size(); ++i) {
        firstLogTime = ParseTime(buffer + lineFeedPos[i], timeFormat);
        if (firstLogTime != -1) {
            firstLogIndex = i;
            break;
        }
    }
    if (firstLogTime >= bootTime) {
        parsedTime = firstLogTime;
        pos = lineFeedPos[firstLogIndex];
        return 1;
    }

    for (int i = (int)lineFeedPos.size() - 1; i >= 0; --i) {
        lastLogTime = ParseTime(buffer + lineFeedPos[i], timeFormat);
        if (lastLogTime != -1) {
            lastLogIndex = (size_t)i;
            break;
        }
    }
    if (lastLogTime < bootTime) {
        parsedTime = lastLogTime;
        pos = lineFeedPos[lastLogIndex];
        return 2;
    }

    // parse all lines, now fisrtLogTime < bootTime, lastLogTime >= booTime
    for (size_t i = firstLogIndex + 1; i < lastLogIndex; ++i) {
        parsedTime = ParseTime(buffer + lineFeedPos[i], timeFormat);
        if (parsedTime >= bootTime) {
            pos = lineFeedPos[i];
            return 0;
        }
    }

    parsedTime = lastLogTime;
    pos = lineFeedPos[lastLogIndex];
    return 0;
}

int32_t LogFileReader::ParseTime(const char* buffer, const std::string& timeFormat) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    long nanosecond = 0;
    int nanosecondLength;
    const char* result = strptime_ns(buffer, timeFormat.c_str(), &tm, &nanosecond, &nanosecondLength);
    tm.tm_isdst = -1;
    if (result != NULL) {
        time_t logTime = mktime(&tm);
        return logTime;
    }
    return -1;
}

bool LogFileReader::CheckForFirstOpen(FileReadPolicy policy) {
    mFirstWatched = false;
    if (mLastFilePos != 0)
        return false;

    // here we should NOT use mLogFileOp to open file, because LogFileReader may be created from checkpoint
    // we just want to set file pos, then a TEMPORARY object for LogFileOperator is needed here, not a class member
    // LogFileOperator we should open file via UpdateFilePtr, then start reading
    LogFileOperator op;
    op.Open(mHostLogPath.c_str(), mIsFuseMode);
    if (op.IsOpen() == false) {
        mLastFilePos = 0;
        mCache.clear();
        LOG_INFO(sLogger,
                 ("force reading file from the beginning",
                  "open file failed when trying to find the start position for reading")("project", mProjectName)(
                     "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                     "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                     "file signature", mLastFileSignatureHash)("file signature size",
                                                               mLastFileSignatureSize)("file size", mLastFileSize));
        auto error = GetErrno();
        if (fsutil::Dir::IsENOENT(error))
            return true;
        else {
            LOG_ERROR(sLogger, ("open log file fail", mHostLogPath)("errno", ErrnoToString(error)));
            LogtailAlarm::GetInstance()->SendAlarm(OPEN_LOGFILE_FAIL_ALARM,
                                                   string("Failed to open log file: ") + mHostLogPath
                                                       + "; errono:" + ErrnoToString(error),
                                                   mProjectName,
                                                   mCategory,
                                                   mRegion);
            return false;
        }
    }

    if (policy == BACKWARD_TO_FIXED_POS) {
        SetFilePosBackwardToFixedPos(op);
    } else if (policy == BACKWARD_TO_BOOT_TIME) {
        bool succeeded = SetReadPosForBackwardReading(op);
        if (!succeeded) {
            // fallback
            SetFilePosBackwardToFixedPos(op);
        }
    } else if (policy == BACKWARD_TO_BEGINNING) {
        mLastFilePos = 0;
        mCache.clear();
    } else {
        LOG_ERROR(sLogger, ("invalid file read policy for file", mHostLogPath));
        return false;
    }
    LOG_INFO(sLogger,
             ("set the starting position for reading, project", mProjectName)("logstore", mCategory)(
                 "config", mConfigName)("log reader queue name", mHostLogPath)("file device", ToString(mDevInode.dev))(
                 "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                 "file signature size", mLastFileSignatureSize)("start position", mLastFilePos));
    return true;
}

void LogFileReader::SetFilePosBackwardToFixedPos(LogFileOperator& op) {
    int64_t endOffset = op.GetFileSize();
    mLastFilePos = endOffset <= ((int64_t)mTailLimit * 1024) ? 0 : (endOffset - ((int64_t)mTailLimit * 1024));
    mCache.clear();
    FixLastFilePos(op, endOffset);
}

void LogFileReader::FixLastFilePos(LogFileOperator& op, int64_t endOffset) {
    if (mLastFilePos == 0 || op.IsOpen() == false) {
        return;
    }
    int32_t readSize = endOffset - mLastFilePos < INT32_FLAG(max_fix_pos_bytes) ? endOffset - mLastFilePos
                                                                                : INT32_FLAG(max_fix_pos_bytes);
    char* readBuf = (char*)malloc(readSize + 1);
    memset(readBuf, 0, readSize + 1);
    size_t readSizeReal = ReadFile(op, readBuf, readSize, mLastFilePos);
    if (readSizeReal == (size_t)0) {
        free(readBuf);
        return;
    }
    for (size_t i = 0; i < readSizeReal - 1; ++i) {
        if (readBuf[i] == '\n') {
            if (mLogBeginRegPtr == NULL) {
                mLastFilePos += i + 1;
                mCache.clear();
                free(readBuf);
                return;
            }
            // cast '\n' to '\0'
            readBuf[i] = '\0';
        }
    }
    string exception;
    if (mLogBeginRegPtr != NULL) {
        for (size_t i = 0; i < readSizeReal - 1; ++i) {
            if (readBuf[i] == '\0' && BoostRegexMatch(readBuf + i + 1, readSize - i - 1, *mLogBeginRegPtr, exception)) {
                mLastFilePos += i + 1;
                mCache.clear();
                free(readBuf);
                return;
            }
        }
    }

    LOG_WARNING(sLogger,
                ("no begin line found", "most likely to have parse error when reading begins")("project", mProjectName)(
                    "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                    "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                    "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                    "search start position", mLastFilePos)("search end position", mLastFilePos + readSizeReal));

    free(readBuf);
    return;
}

std::string LogFileReader::GetTopicName(const std::string& topicConfig, const std::string& path) {
    std::string finalPath = mDockerPath.size() > 0 ? mDockerPath : path;
    size_t len = finalPath.size();
    // ignore the ".1" like suffix when the log file is roll back
    if (len > 2 && finalPath[len - 2] == '.' && finalPath[len - 1] > '0' && finalPath[len - 1] < '9') {
        finalPath = finalPath.substr(0, len - 2);
    }

    {
        string res;
        std::vector<string> keys;
        std::vector<string> values;
        if (ExtractTopics(finalPath, topicConfig, keys, values)) {
            size_t matchedSize = values.size();
            if (matchedSize == (size_t)1) {
                // != default topic name
                if (keys[0] != "__topic_1__") {
                    sls_logs::LogTag tag;
                    tag.set_key(keys[0]);
                    tag.set_value(values[0]);
                    mExtraTags.push_back(tag);
                }
                return values[0];
            } else {
                for (size_t i = 0; i < matchedSize; ++i) {
                    if (res.empty()) {
                        res = values[i];
                    } else {
                        res = res + "_" + values[i];
                    }
                    sls_logs::LogTag tag;
                    tag.set_key(keys[i]);
                    tag.set_value(values[i]);
                    mExtraTags.push_back(tag);
                }
            }
            return res;
        }
    }

    boost::match_results<const char*> what;
    string res, exception;
    // catch exception
    boost::regex topicRegex;
    try {
        topicRegex = boost::regex(topicConfig.c_str());
        if (BoostRegexMatch(finalPath.c_str(), finalPath.length(), topicRegex, exception, what, boost::match_default)) {
            size_t matchedSize = what.size();
            for (size_t i = 1; i < matchedSize; ++i) {
                if (res.empty()) {
                    res = what[i];
                } else {
                    res = res + "_" + what[i];
                }
                if (matchedSize > 2) {
                    sls_logs::LogTag tag;
                    tag.set_key(string("__topic_") + ToString(i) + "__");
                    tag.set_value(what[i]);
                    mExtraTags.push_back(tag);
                }
            }
        } else {
            if (!exception.empty())
                LOG_ERROR(sLogger,
                          ("extract topic by regex", "fail")("exception", exception)("project", mProjectName)(
                              "logstore", mCategory)("path", finalPath)("regx", topicConfig));
            else
                LOG_WARNING(sLogger,
                            ("extract topic by regex", "fail")("project", mProjectName)("logstore", mCategory)(
                                "path", finalPath)("regx", topicConfig));

            LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
                                                   string("extract topic by regex fail, exception:") + exception
                                                       + ", path:" + finalPath + ", regex:" + topicConfig,
                                                   mProjectName,
                                                   mCategory,
                                                   mRegion);
        }
    } catch (...) {
        LOG_ERROR(sLogger,
                  ("extract topic by regex", "fail")("exception", exception)("project", mProjectName)(
                      "logstore", mCategory)("path", finalPath)("regx", topicConfig));
        LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
                                               string("extract topic by regex fail, exception:") + exception
                                                   + ", path:" + finalPath + ", regex:" + topicConfig,
                                               mProjectName,
                                               mCategory,
                                               mRegion);
    }

    return res;
}

RangeCheckpointPtr LogFileReader::selectCheckpointToReplay() {
    if (mEOOption->toReplayCheckpoints.empty()) {
        return nullptr;
    }

    do {
        auto& last = mEOOption->toReplayCheckpoints.back()->data;
        if (static_cast<int64_t>(last.read_offset() + last.read_length()) > mLastFileSize) {
            LOG_ERROR(sLogger,
                      ("current file size does not match last checkpoint",
                       "")COMMON_READER_INFO("file size", mLastFileSize)("checkpoint", last.DebugString()));
            break;
        }
        auto& first = mEOOption->toReplayCheckpoints.front()->data;
        if (static_cast<int64_t>(first.read_offset()) != mLastFilePos) {
            LOG_ERROR(sLogger,
                      ("current offset does not match first checkpoint",
                       "")COMMON_READER_INFO("offset", mLastFilePos)("checkpoint", first.DebugString()));
            break;
        }

        auto cpt = mEOOption->toReplayCheckpoints.front();
        mEOOption->toReplayCheckpoints.pop_front();
        return cpt;
    } while (false);

    LOG_ERROR(sLogger, COMMON_READER_INFO("delete all checkpoints to replay", mEOOption->toReplayCheckpoints.size()));
    for (auto& cpt : mEOOption->toReplayCheckpoints) {
        cpt->IncreaseSequenceID();
    }
    mEOOption->toReplayCheckpoints.clear();
    return nullptr;
}

void LogFileReader::skipCheckpointRelayHole() {
    if (mEOOption->toReplayCheckpoints.empty()) {
        if (mEOOption->lastComittedOffset != -1 && mEOOption->lastComittedOffset > mLastFilePos) {
            LOG_INFO(sLogger,
                     COMMON_READER_INFO("no more checkpoint to replay", "skip to last committed offset")(
                         "offset", mEOOption->lastComittedOffset)("current", mLastFilePos));
            mLastFilePos = mEOOption->lastComittedOffset;
            mEOOption->lastComittedOffset = -1;
        }
        return;
    }

    auto& next = mEOOption->toReplayCheckpoints.front()->data;
    auto const readOffset = static_cast<int64_t>(next.read_offset());
    if (readOffset == mLastFilePos) {
        return;
    }
    LOG_INFO(sLogger,
             ("skip replay hole for next checkpoint, size", readOffset - mLastFilePos)
                 COMMON_READER_INFO("offset", mLastFilePos)("checkpoint", next.DebugString()));
    mLastFilePos = readOffset;
}

bool LogFileReader::ReadLog(LogBuffer& logBuffer, const Event* event) {
    if (mLogFileOp.IsOpen() == false) {
        if (!ShouldForceReleaseDeletedFileFd()) {
            // should never happen
            LOG_ERROR(sLogger, ("unknow error, log file not open", mHostLogPath));
        }
        return false;
    }
    if (AppConfig::GetInstance()->IsInputFlowControl())
        LogInput::GetInstance()->FlowControl();

    if (mFirstWatched && (mLastFilePos == 0))
        CheckForFirstOpen();

    // Init checkpoint for this read, new if no checkpoint to replay.
    if (mEOOption) {
        mEOOption->selectedCheckpoint = selectCheckpointToReplay();
        if (!mEOOption->selectedCheckpoint) {
            mEOOption->selectedCheckpoint.reset(new RangeCheckpoint);
            mEOOption->selectedCheckpoint->fbKey = mEOOption->fbKey;
        }
    }

    size_t lastFilePos = mLastFilePos;
    bool allowRollback = true;
    if (event != nullptr && event->IsReaderFlushTimeout()) {
        // If flush timeout event, we should filter whether the event is legacy.
        if (event->GetLastReadPos() == GetLastReadPos() && event->GetLastFilePos() == mLastFilePos
            && event->GetInode() == mDevInode.inode) {
            allowRollback = false;
        } else {
            return false;
        }
    }
    bool moreData = GetRawData(logBuffer, mLastFileSize, allowRollback);
    if (!logBuffer.rawBuffer.empty() > 0) {
        if (mEOOption) {
            // This read was replayed by checkpoint, adjust mLastFilePos to skip hole.
            if (mEOOption->selectedCheckpoint->IsComplete()) {
                skipCheckpointRelayHole();
            }
            logBuffer.exactlyOnceCheckpoint = mEOOption->selectedCheckpoint;
        }
    }
    LOG_DEBUG(sLogger,
              ("read log file", mRealLogPath)("last file pos", mLastFilePos)("last file size", mLastFileSize)(
                  "read size", mLastFilePos - lastFilePos));
    if (HasDataInCache()) {
        auto event = CreateFlushTimeoutEvent();
        BlockedEventManager::GetInstance()->UpdateBlockEvent(
            GetLogstoreKey(), mConfigName, *event, mDevInode, time(NULL) + mReaderFlushTimeout);
    }
    return moreData;
}

void LogFileReader::OnOpenFileError() {
    switch (errno) {
        case ENOENT:
            LOG_INFO(sLogger,
                     ("open file failed", " log file not exist, probably caused by rollback")("project", mProjectName)(
                         "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                         "log path", mRealLogPath)("file device", ToString(mDevInode.dev))(
                         "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                         "file signature size", mLastFileSignatureSize)("last file position", mLastFilePos));
            break;
        case EACCES:
            LOG_ERROR(sLogger,
                      ("open file failed", "open log file fail because of permission")("project", mProjectName)(
                          "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                          "log path", mRealLogPath)("file device", ToString(mDevInode.dev))(
                          "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                          "file signature size", mLastFileSignatureSize)("last file position", mLastFilePos));
            LogtailAlarm::GetInstance()->SendAlarm(LOGFILE_PERMINSSION_ALARM,
                                                   string("Failed to open log file because of permission: ")
                                                       + mHostLogPath,
                                                   mProjectName,
                                                   mCategory,
                                                   mRegion);
            break;
        case EMFILE:
            LOG_ERROR(sLogger,
                      ("open file failed", "too many open file")("project", mProjectName)("logstore", mCategory)(
                          "config", mConfigName)("log reader queue name", mHostLogPath)("log path", mRealLogPath)(
                          "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                          "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                          "last file position", mLastFilePos));
            LogtailAlarm::GetInstance()->SendAlarm(OPEN_LOGFILE_FAIL_ALARM,
                                                   string("Failed to open log file because of : Too many open files")
                                                       + mHostLogPath,
                                                   mProjectName,
                                                   mCategory,
                                                   mRegion);
            break;
        default:
            LOG_ERROR(sLogger,
                      ("open file failed, errno", ErrnoToString(GetErrno()))("logstore", mCategory)(
                          "config", mConfigName)("log reader queue name", mHostLogPath)("log path", mRealLogPath)(
                          "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                          "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                          "last file position", mLastFilePos));
            LogtailAlarm::GetInstance()->SendAlarm(OPEN_LOGFILE_FAIL_ALARM,
                                                   string("Failed to open log file: ") + mHostLogPath
                                                       + "; errono:" + ErrnoToString(GetErrno()),
                                                   mProjectName,
                                                   mCategory,
                                                   mRegion);
    }
}

bool LogFileReader::UpdateFilePtr() {
    // move last update time before check IsValidToPush
    mLastUpdateTime = time(NULL);
    if (mLogFileOp.IsOpen() == false) {
        // In several cases we should revert file deletion flag:
        // 1. File is appended after deletion. This happens when app is still logging, but a user deleted the log file.
        // 2. File was rename/moved, but is rename/moved back later.
        // 3. Log rotated. But iLogtail's logic will not remove the reader from readerArray on delete event.
        //    It will be removed while the new file has modify event. The reader is still the head of readerArray,
        //    thus it will be open again for reading.
        // However, if the user explicitly set a delete timeout. We should not revert the flag.
        if (INT32_FLAG(force_release_deleted_file_fd_timeout) < 0) {
            SetFileDeleted(false);
        }
        if (GloablFileDescriptorManager::GetInstance()->GetOpenedFilePtrSize() > INT32_FLAG(max_reader_open_files)) {
            LOG_ERROR(sLogger,
                      ("open file failed, opened fd exceed limit, too many open files",
                       GloablFileDescriptorManager::GetInstance()->GetOpenedFilePtrSize())(
                          "limit", INT32_FLAG(max_reader_open_files))("project", mProjectName)("logstore", mCategory)(
                          "config", mConfigName)("log reader queue name", mHostLogPath)("file device",
                                                                                        ToString(mDevInode.dev))(
                          "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                          "file signature size", mLastFileSignatureSize)("last file position", mLastFilePos));
            LogtailAlarm::GetInstance()->SendAlarm(OPEN_FILE_LIMIT_ALARM,
                                                   string("Failed to open log file: ") + mHostLogPath
                                                       + " limit:" + ToString(INT32_FLAG(max_reader_open_files)),
                                                   mProjectName,
                                                   mCategory,
                                                   mRegion);
            // set errno to "too many open file"
            errno = EMFILE;
            return false;
        }
        int32_t tryTime = 0;
        LOG_DEBUG(sLogger, ("UpdateFilePtr open log file ", mHostLogPath));
        if (mRealLogPath.size() > 0) {
            while (tryTime++ < 5) {
                mLogFileOp.Open(mRealLogPath.c_str(), mIsFuseMode);
                if (mLogFileOp.IsOpen() == false) {
                    usleep(100);
                } else {
                    break;
                }
            }
            if (mLogFileOp.IsOpen() == false) {
                OnOpenFileError();
            } else if (CheckDevInode()) {
                GloablFileDescriptorManager::GetInstance()->OnFileOpen(this);
                LOG_INFO(sLogger,
                         ("open file succeeded, project", mProjectName)("logstore", mCategory)("config", mConfigName)(
                             "log reader queue name", mHostLogPath)("real file path", mRealLogPath)(
                             "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                             "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                             "last file position", mLastFilePos)("reader id", long(this)));
                return true;
            } else {
                mLogFileOp.Close();
            }
        }
        if (mRealLogPath == mHostLogPath) {
            LOG_INFO(sLogger,
                     ("open file failed, log file dev inode changed or file deleted ",
                      "prepare to delete reader or put reader into rotated map")("project", mProjectName)(
                         "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                         "log path", mRealLogPath)("file device", ToString(mDevInode.dev))(
                         "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                         "file signature size", mLastFileSignatureSize)("last file position", mLastFilePos));
            return false;
        }
        tryTime = 0;
        while (tryTime++ < 5) {
            mLogFileOp.Open(mHostLogPath.c_str(), mIsFuseMode);
            if (mLogFileOp.IsOpen() == false) {
                usleep(100);
            } else {
                break;
            }
        }
        if (mLogFileOp.IsOpen() == false) {
            OnOpenFileError();
            return false;
        } else if (CheckDevInode()) {
            // the mHostLogPath's dev inode equal to mDevInode, so real log path is mHostLogPath
            mRealLogPath = mHostLogPath;
            GloablFileDescriptorManager::GetInstance()->OnFileOpen(this);
            LOG_INFO(sLogger,
                     ("open file succeeded, project", mProjectName)("logstore", mCategory)("config", mConfigName)(
                         "log reader queue name", mHostLogPath)("real file path", mRealLogPath)(
                         "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                         "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                         "last file position", mLastFilePos)("reader id", long(this)));
            return true;
        } else {
            mLogFileOp.Close();
        }
        LOG_INFO(sLogger,
                 ("open file failed, log file dev inode changed or file deleted ",
                  "prepare to delete reader")("project", mProjectName)("logstore", mCategory)("config", mConfigName)(
                     "log reader queue name", mHostLogPath)("log path", mRealLogPath)("file device",
                                                                                      ToString(mDevInode.dev))(
                     "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                     "file signature size", mLastFileSignatureSize)("last file position", mLastFilePos));
        return false;
    }
    return true;
}

bool LogFileReader::CloseTimeoutFilePtr(int32_t curTime) {
    int32_t timeOut = (int32_t)(mCloseUnusedInterval / 100.f * (100 + rand() % 50));
    if (mLogFileOp.IsOpen() && curTime - mLastUpdateTime > timeOut) {
        fsutil::PathStat buf;
        if (mLogFileOp.Stat(buf) != 0) {
            return false;
        }
        if ((int64_t)buf.GetFileSize() == mLastFilePos) {
            LOG_INFO(sLogger,
                     ("close the file", "current log file has not been updated for some time and has been read")(
                         "project", mProjectName)("logstore", mCategory)("config", mConfigName)("log reader queue name",
                                                                                                mHostLogPath)(
                         "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                         "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                         "file size", mLastFileSize)("last file position", mLastFilePos));
            CloseFilePtr();
            // delete item in LogFileCollectOffsetIndicator map
            LogFileCollectOffsetIndicator::GetInstance()->DeleteItem(mHostLogPath, mDevInode);
            return true;
        }
    }
    return false;
}

void LogFileReader::CloseFilePtr() {
    if (mLogFileOp.IsOpen()) {
        mCache.shrink_to_fit();
        LOG_DEBUG(sLogger, ("start close LogFileReader", mHostLogPath));

        // if mHostLogPath is symbolic link, then we should not update it accrding to /dev/fd/xx
        if (!mSymbolicLinkFlag) {
            // retrieve file path from file descriptor in order to open it later
            // this is important when file is moved when rotating
            string curRealLogPath = mLogFileOp.GetFilePath();
            if (!curRealLogPath.empty()) {
                LOG_INFO(sLogger,
                         ("update the real file path of the log reader during closing, project", mProjectName)(
                             "logstore", mCategory)("config", mConfigName)("log reader queue name", mHostLogPath)(
                             "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                             "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                             "original file path", mRealLogPath)("new file path", curRealLogPath));
                mRealLogPath = curRealLogPath;
                if (mEOOption && mRealLogPath != mEOOption->primaryCheckpoint.real_path()) {
                    updatePrimaryCheckpointRealPath();
                }
            } else {
                LOG_WARNING(sLogger, ("failed to get real log path", mHostLogPath));
            }
        }

        if (mLogFileOp.Close() != 0) {
            int fd = mLogFileOp.GetFd();
            LOG_WARNING(
                sLogger,
                ("close file error", strerror(errno))("fd", fd)("project", mProjectName)("logstore", mCategory)(
                    "config", mConfigName)("log reader queue name", mHostLogPath)("real file path", mRealLogPath)(
                    "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                    "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                    "file size", mLastFileSize)("last file position", mLastFilePos)("reader id", long(this)));
            LogtailAlarm::GetInstance()->SendAlarm(OPEN_LOGFILE_FAIL_ALARM,
                                                   string("close file error because of ") + strerror(errno)
                                                       + ", file path: " + mHostLogPath + ", inode: "
                                                       + ToString(mDevInode.inode) + ", inode: " + ToString(fd),
                                                   mProjectName,
                                                   mCategory,
                                                   mRegion);
        } else {
            LOG_INFO(sLogger,
                     ("close file succeeded, project", mProjectName)("logstore", mCategory)("config", mConfigName)(
                         "log reader queue name", mHostLogPath)("real file path", mRealLogPath)(
                         "file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                         "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                         "file size", mLastFileSize)("last file position", mLastFilePos)("reader id", long(this)));
        }
        // always call OnFileClose
        GloablFileDescriptorManager::GetInstance()->OnFileClose(this);
    }
}

uint64_t LogFileReader::GetLogstoreKey() const {
    return mEOOption ? mEOOption->fbKey : mLogstoreKey;
}

bool LogFileReader::CheckDevInode() {
    fsutil::PathStat statBuf;
    if (mLogFileOp.Stat(statBuf) != 0) {
        if (errno == ENOENT) {
            LOG_WARNING(sLogger, ("file deleted ", "unknow error")("path", mHostLogPath)("fd", mLogFileOp.GetFd()));
        } else {
            LOG_WARNING(sLogger,
                        ("get file info error, ", strerror(errno))("path", mHostLogPath)("fd", mLogFileOp.GetFd()));
        }
        return false;
    } else {
        DevInode devInode = statBuf.GetDevInode();
        return devInode == mDevInode;
    }
}

bool LogFileReader::CheckFileSignatureAndOffset(bool isOpenOnUpdate) {
    mLastEventTime = time(NULL);
    int64_t endSize = mLogFileOp.GetFileSize();
    if (endSize < 0) {
        int lastErrNo = errno;
        if (mLogFileOp.Close() == 0) {
            LOG_INFO(sLogger,
                     ("close file succeeded, project", mProjectName)("logstore", mCategory)("config", mConfigName)(
                         "log reader queue name",
                         mHostLogPath)("file device", ToString(mDevInode.dev))("file inode", ToString(mDevInode.inode))(
                         "file signature", mLastFileSignatureHash)("file signature size", mLastFileSignatureSize)(
                         "file size", mLastFileSize)("last file position", mLastFilePos));
        }
        GloablFileDescriptorManager::GetInstance()->OnFileClose(this);
        bool reopenFlag = UpdateFilePtr();
        endSize = mLogFileOp.GetFileSize();
        LOG_WARNING(sLogger,
                    ("tell error", mHostLogPath)("inode", mDevInode.inode)("error", strerror(lastErrNo))(
                        "reopen", reopenFlag)("project", mProjectName)("logstore", mCategory)("config", mConfigName));
        LogtailAlarm::GetInstance()->SendAlarm(OPEN_LOGFILE_FAIL_ALARM,
                                               string("tell error because of ") + strerror(lastErrNo) + " file path: "
                                                   + mHostLogPath + ", inode : " + ToString(mDevInode.inode),
                                               mProjectName,
                                               mCategory,
                                               mRegion);
        if (endSize < 0) {
            return false;
        }
    }
    mLastFileSize = endSize;

    // If file size is 0 and filename is changed, we cannot judge if the inode is reused by signature,
    // so we just recreate the reader to avoid filename mismatch
    if (mLastFileSignatureSize == 0 && mRealLogPath != mHostLogPath) {
        return false;
    }
    fsutil::PathStat ps;
    mLogFileOp.Stat(ps);
    time_t lastMTime = mLastMTime;
    mLastMTime = ps.GetMtime();
    if (!isOpenOnUpdate || mLastFileSignatureSize == 0 || endSize < mLastFilePos || (endSize == mLastFilePos && lastMTime != mLastMTime)) {
        char firstLine[1025];
        int nbytes = mLogFileOp.Pread(firstLine, 1, 1024, 0);
        if (nbytes < 0) {
            LOG_ERROR(sLogger,
                      ("fail to read file", mHostLogPath)("nbytes", nbytes)("project", mProjectName)(
                          "logstore", mCategory)("config", mConfigName));
            return false;
        }
        firstLine[nbytes] = '\0';
        bool sigCheckRst = CheckAndUpdateSignature(string(firstLine), mLastFileSignatureHash, mLastFileSignatureSize);
        if (!sigCheckRst) {
            LOG_INFO(sLogger,
                     ("Check file truncate by signature, read from begin",
                      mHostLogPath)("project", mProjectName)("logstore", mCategory)("config", mConfigName));
            mLastFilePos = 0;
            if (mEOOption) {
                updatePrimaryCheckpointSignature();
            }
            return false;
        } else if (mEOOption && mEOOption->primaryCheckpoint.sig_size() != mLastFileSignatureSize) {
            updatePrimaryCheckpointSignature();
        }
    }

    if (endSize < mLastFilePos) {
        LOG_INFO(sLogger,
                 ("File signature is same but size decrease, read from now fileSize",
                  mHostLogPath)(ToString(endSize), ToString(mLastFilePos))("project", mProjectName)(
                     "logstore", mCategory)("config", mConfigName));

        LogtailAlarm::GetInstance()->SendAlarm(LOG_TRUNCATE_ALARM,
                                               mHostLogPath
                                                   + " signature is same but size decrease, read from now fileSize "
                                                   + ToString(endSize) + " last read pos " + ToString(mLastFilePos),
                                               mProjectName,
                                               mCategory,
                                               mRegion);

        mLastFilePos = endSize;
        // when we use truncate_pos_skip_bytes, if truncate stop and log start to append, logtail will drop less data or
        // collect more data this just work around for ant's demand
        if (INT32_FLAG(truncate_pos_skip_bytes) > 0 && mLastFilePos > (INT32_FLAG(truncate_pos_skip_bytes) + 1024)) {
            mLastFilePos -= INT32_FLAG(truncate_pos_skip_bytes);
            // after adjust mLastFilePos, we should fix last pos to assure that each log is complete
            FixLastFilePos(mLogFileOp, endSize);
        }
    }
    return true;
}

LogFileReaderPtrArray* LogFileReader::GetReaderArray() {
    return mReaderArray;
}

void LogFileReader::SetReaderArray(LogFileReaderPtrArray* readerArray) {
    mReaderArray = readerArray;
}

void LogFileReader::ResetTopic(const std::string& topicFormat) {
    const std::string lowerConfig = ToLowerCaseString(topicFormat);
    if (lowerConfig == "none" || lowerConfig == "default" || lowerConfig == "global_topic"
        || lowerConfig == "group_topic" || lowerConfig == "customized") {
        return;
    } else {
        // only reset file's topic
        mTopicName = GetTopicName(topicFormat, mHostLogPath);
    }
}

void LogFileReader::SetReadBufferSize(int32_t bufSize) {
    if (bufSize < 1024 * 10 || bufSize > 1024 * 1024 * 1024) {
        LOG_ERROR(sLogger, ("invalid read buffer size", bufSize));
        return;
    }
    LOG_INFO(sLogger, ("set max read buffer size", bufSize));
    BUFFER_SIZE = bufSize;
}

bool LogFileReader::LogSplit(const char* buffer,
                             int32_t size,
                             int32_t& lineFeed,
                             std::vector<StringView>& logIndex,
                             std::vector<StringView>& discardIndex) {
    /*
               | -------------- | -------- \n
        multiBeginIndex      begIndex   endIndex

        multiBeginIndex: used to cache current parsing log. Clear when starting the next log.
        begIndex: the begin index of the current line
        endIndex: the end index of the current line

        Supported regex combination:
        1. begin
        2. begin + continue
        3. begin + end
        4. continue + end
        5. end
    */
    int multiBeginIndex = 0;
    int begIndex = 0;
    int endIndex = 0;
    bool anyMatched = false;
    lineFeed = 0;
    std::string exception;
    SplitState state = SPLIT_UNMATCH;
    while (endIndex <= size) {
        if (endIndex == size || buffer[endIndex] == '\n') {
            lineFeed++;
            exception.clear();
            // State machine with three states (SPLIT_UNMATCH, SPLIT_BEGIN, SPLIT_CONTINUE)
            switch (state) {
                case SPLIT_UNMATCH:
                    if (!IsMultiLine()) {
                        // Single line log
                        anyMatched = true;
                        logIndex.emplace_back(buffer + begIndex, endIndex - begIndex);
                        multiBeginIndex = endIndex + 1;
                        break;
                    } else if (mLogBeginRegPtr != NULL) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogBeginRegPtr, exception)) {
                            // Just clear old cache, task current line as the new cache
                            if (multiBeginIndex != begIndex) {
                                anyMatched = true;
                                logIndex[logIndex.size() - 1] = StringView(logIndex[logIndex.size() - 1].begin(),
                                                                           logIndex[logIndex.size() - 1].length()
                                                                               + begIndex - 1 - multiBeginIndex);
                                multiBeginIndex = begIndex;
                            }
                            state = SPLIT_BEGIN;
                            break;
                        }
                        handleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                        break;
                    }
                    // mLogContinueRegPtr can be matched 0 or multiple times, if not match continue to try mLogEndRegPtr
                    if (mLogContinueRegPtr != NULL
                        && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogContinueRegPtr, exception)) {
                        state = SPLIT_CONTINUE;
                        break;
                    }
                    if (mLogEndRegPtr != NULL
                        && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogEndRegPtr, exception)) {
                        // output logs in cache from multiBeginIndex to endIndex
                        anyMatched = true;
                        logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
                        multiBeginIndex = endIndex + 1;
                        break;
                    }
                    handleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                    break;

                case SPLIT_BEGIN:
                    // mLogContinueRegPtr can be matched 0 or multiple times, if not match continue to try others.
                    if (mLogContinueRegPtr != NULL
                        && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogContinueRegPtr, exception)) {
                        state = SPLIT_CONTINUE;
                        break;
                    }
                    if (mLogEndRegPtr != NULL) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogEndRegPtr, exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
                            multiBeginIndex = endIndex + 1;
                            state = SPLIT_UNMATCH;
                        }
                        // for case: begin unmatch end
                        // so logs cannot be handled as unmatch even if not match LogEngReg
                    } else if (mLogBeginRegPtr != NULL) {
                        anyMatched = true;
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogBeginRegPtr, exception)) {
                            if (multiBeginIndex != begIndex) {
                                logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                                multiBeginIndex = begIndex;
                            }
                        } else if (mLogContinueRegPtr != NULL) {
                            // case: begin+continue, but we meet unmatch log here
                            logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                            multiBeginIndex = begIndex;
                            handleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                            state = SPLIT_UNMATCH;
                        }
                        // else case: begin+end or begin, we should keep unmatch log in the cache
                    }
                    break;

                case SPLIT_CONTINUE:
                    // mLogContinueRegPtr can be matched 0 or multiple times, if not match continue to try others.
                    if (mLogContinueRegPtr != NULL
                        && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogContinueRegPtr, exception)) {
                        break;
                    }
                    if (mLogEndRegPtr != NULL) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogEndRegPtr, exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
                            multiBeginIndex = endIndex + 1;
                            state = SPLIT_UNMATCH;
                        } else {
                            handleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                            state = SPLIT_UNMATCH;
                        }
                    } else if (mLogBeginRegPtr != NULL) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogBeginRegPtr, exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                            multiBeginIndex = begIndex;
                            state = SPLIT_BEGIN;
                        } else {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                            multiBeginIndex = begIndex;
                            handleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                            state = SPLIT_UNMATCH;
                        }
                    } else {
                        anyMatched = true;
                        logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                        multiBeginIndex = begIndex;
                        handleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                        state = SPLIT_UNMATCH;
                    }
                    break;
            }
            begIndex = endIndex + 1;
            if (!exception.empty()) {
                if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                    if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                        LOG_ERROR(sLogger,
                                  ("regex_match in LogSplit fail, exception",
                                   exception)("project", mProjectName)("logstore", mCategory)("file", mHostLogPath));
                    }
                    LogtailAlarm::GetInstance()->SendAlarm(REGEX_MATCH_ALARM,
                                                           "regex_match in LogSplit fail:" + exception,
                                                           mProjectName,
                                                           mCategory,
                                                           mRegion);
                }
            }
        }
        endIndex++;
    }
    // We should clear the log from `multiBeginIndex` to `size`.
    if (multiBeginIndex < size) {
        if (!IsMultiLine()) {
            logIndex.emplace_back(buffer + multiBeginIndex, size - multiBeginIndex);
        } else {
            endIndex = buffer[size - 1] == '\n' ? size - 1 : size;
            if (mLogBeginRegPtr != NULL && mLogEndRegPtr == NULL) {
                anyMatched = true;
                // If logs is unmatched, they have been handled immediately. So logs must be matched here.
                logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
            } else if (mLogBeginRegPtr == NULL && mLogContinueRegPtr == NULL && mLogEndRegPtr != NULL) {
                // If there is still logs in cache, it means that there is no end line. We can handle them as unmatched.
                if (mDiscardUnmatch) {
                    for (int i = multiBeginIndex; i <= endIndex; i++) {
                        if (i == endIndex || buffer[i] == '\n') {
                            discardIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                            multiBeginIndex = i + 1;
                        }
                    }
                } else {
                    for (int i = multiBeginIndex; i <= endIndex; i++) {
                        if (i == endIndex || buffer[i] == '\n') {
                            logIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                            multiBeginIndex = i + 1;
                        }
                    }
                }
            } else {
                handleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
            }
        }
    }
    return anyMatched;
}

void LogFileReader::handleUnmatchLogs(const char* buffer,
                                      int& multiBeginIndex,
                                      int endIndex,
                                      std::vector<StringView>& logIndex,
                                      std::vector<StringView>& discardIndex) {
    // Cannot determine where log is unmatched here where there is only mLogEndRegPtr
    if (mLogBeginRegPtr == NULL && mLogContinueRegPtr == NULL && mLogEndRegPtr != NULL) {
        return;
    }
    if (mDiscardUnmatch) {
        for (int i = multiBeginIndex; i <= endIndex; i++) {
            if (i == endIndex || buffer[i] == '\n') {
                discardIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                multiBeginIndex = i + 1;
            }
        }
    } else {
        for (int i = multiBeginIndex; i <= endIndex; i++) {
            if (i == endIndex || buffer[i] == '\n') {
                logIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                multiBeginIndex = i + 1;
            }
        }
    }
}

bool LogFileReader::ParseLogTime(const char* buffer,
                                 const boost::regex* reg,
                                 LogtailTime& logTime,
                                 const std::string& timeFormat,
                                 const std::string& region,
                                 const std::string& project,
                                 const std::string& logStore,
                                 const std::string& logPath) {
    std::string exception;
    boost::match_results<const char*> what;
    if (reg != NULL && BoostRegexSearch(buffer, *reg, exception, what, boost::match_default)) {
        if (!what.empty()) {
            std::string timeStr(what[0].str());
            // convert log time
            struct tm t;
            memset(&t, 0, sizeof(t));
            int nanosecondLength;
            if (strptime_ns(timeStr.c_str(), timeFormat.c_str(), &t, &logTime.tv_nsec, &nanosecondLength) == NULL) {
                LOG_ERROR(sLogger,
                          ("convert time failed, time str", timeStr)("time format", timeFormat)("project", project)(
                              "logstore", logStore)("file", logPath));
                return false;
            }

            t.tm_isdst = -1;
            logTime.tv_sec = mktime(&t);
            return true;
        }
    }
    if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
        if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            LOG_ERROR(sLogger,
                      ("parse regex log fail, exception",
                       exception)("buffer", buffer)("project", project)("logstore", logStore)("file", logPath));
        }
        LogtailAlarm::GetInstance()->SendAlarm(
            REGEX_MATCH_ALARM, "parse regex log fail:" + exception, project, logStore, region);
    }
    return false;
}

bool LogFileReader::GetLogTimeByOffset(const char* buffer,
                                       int32_t pos,
                                       LogtailTime& logTime,
                                       const std::string& timeFormat,
                                       const std::string& region,
                                       const std::string& project,
                                       const std::string& logStore,
                                       const std::string& logPath) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    long nanosecond = 0;
    int nanosecondLength = 0;
    if (strptime_ns(buffer + pos, timeFormat.c_str(), &t, &nanosecond, &nanosecondLength) == NULL) {
        if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
            if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                LOG_WARNING(sLogger,
                            ("get time by offset fail, region", region)("project", project)("logstore",
                                                                                            logStore)("file", logPath));
            }
            LogtailAlarm::GetInstance()->SendAlarm(
                PARSE_TIME_FAIL_ALARM, "errorlog:" + string(buffer), project, logStore, region);
        }
        return false;
    }
    t.tm_isdst = -1;
    logTime.tv_sec = mktime(&t);
    return true;
}

// Only get the currently written log file, it will choose the last modified file to read. There are several condition
// to choose the lastmodify file:
// 1. if the last read file don't exist
// 2. if the file's first 100 bytes(file signature) is not same with the last read file's signature, which meaning the
// log file has be rolled
//
// when a new file is choosen, it will set the read position
// 1. if the time in the file's first line >= the last read log time , then set the file read position to 0 (which mean
// the file is new created)
// 2. other wise , set the position to the end of the file
// *bufferptr is null terminated.
/*
 * 1. for multiline log, "xxx" mean a string without '\n'
 * 1-1. bufferSize = 512KB:
 * "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3\n" -> "MultiLineLog_1\nMultiLineLog_2\0"
 * "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3_Line_1\n" -> "MultiLineLog_1\nMultiLineLog_2\0"
 * "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3_Line_1\nxxx" -> "MultiLineLog_1\nMultiLineLog_2\0"
 * "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3\nxxx" -> "MultiLineLog_1\nMultiLineLog_2\0"
 *
 * 1-2. bufferSize < 512KB:
 * "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3\n" -> "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3\0"
 * "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3_Line_1\n" -> "MultiLineLog_1\nMultiLineLog_2\MultiLineLog_3_Line_1\0"
 * **this is not expected !** "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3_Line_1\nxxx" ->
 * "MultiLineLog_1\nMultiLineLog_2\0" "MultiLineLog_1\nMultiLineLog_2\nMultiLineLog_3\nxxx" ->
 * "MultiLineLog_1\nMultiLineLog_2\0"
 *
 * 2. for singleline log, "xxx" mean a string without '\n'
 * "SingleLineLog_1\nSingleLineLog_2\nSingleLineLog_3\n" -> "SingleLineLog_1\nSingleLineLog_2\nSingleLineLog_3\0"
 * "SingleLineLog_1\nSingleLineLog_2\nxxx" -> "SingleLineLog_1\nSingleLineLog_2\0"
 */
bool LogFileReader::GetRawData(LogBuffer& logBuffer, int64_t fileSize, bool allowRollback) {
    // Truncate, return false to indicate no more data.
    if (fileSize == mLastFilePos) {
        return false;
    }

    bool moreData = false;
    if (mFileEncoding == ENCODING_GBK)
        ReadGBK(logBuffer, fileSize, moreData, allowRollback);
    else
        ReadUTF8(logBuffer, fileSize, moreData, allowRollback);

    int64_t delta = fileSize - mLastFilePos;
    if (delta > mReadDelayAlarmBytes && !logBuffer.rawBuffer.empty()) {
        int32_t curTime = time(NULL);
        if (mReadDelayTime == 0)
            mReadDelayTime = curTime;
        else if (curTime - mReadDelayTime >= INT32_FLAG(read_delay_alarm_duration)) {
            mReadDelayTime = curTime;
            LOG_WARNING(sLogger,
                        ("read log delay", mHostLogPath)("fall behind bytes",
                                                         delta)("file size", fileSize)("read pos", mLastFilePos));
            LogtailAlarm::GetInstance()->SendAlarm(
                READ_LOG_DELAY_ALARM,
                std::string("fall behind ") + ToString(delta) + " bytes, file size:" + ToString(fileSize)
                    + ", now position:" + ToString(mLastFilePos) + ", path:" + mHostLogPath
                    + ", now read log content:" + logBuffer.rawBuffer.substr(0, 256).to_string(),
                mProjectName,
                mCategory,
                mRegion);
        }
    } else
        mReadDelayTime = 0;

    // if delta size > mReadDelaySkipBytes, force set file pos and send alarm
    if (mReadDelaySkipBytes > 0 && delta > mReadDelaySkipBytes) {
        LOG_WARNING(sLogger,
                    ("read log delay and force set file pos to file size", mHostLogPath)("fall behind bytes", delta)(
                        "skip bytes config", mReadDelaySkipBytes)("file size", fileSize)("read pos", mLastFilePos));
        LogtailAlarm::GetInstance()->SendAlarm(
            READ_LOG_DELAY_ALARM,
            string("force set file pos to file size, fall behind ") + ToString(delta)
                + " bytes, file size:" + ToString(fileSize) + ", now position:" + ToString(mLastFilePos)
                + ", path:" + mHostLogPath + ", now read log content:" + logBuffer.rawBuffer.substr(0, 256).to_string(),
            mProjectName,
            mCategory,
            mRegion);
        mLastFilePos = fileSize;
        mCache.clear();
    }

    if (mMarkOffsetFlag && logBuffer.rawBuffer.size() > 0) {
        logBuffer.fileInfo.reset(new FileInfo(mLogFileOp.GetFd(), mDevInode));
        FileInfoPtr& fileInfo = logBuffer.fileInfo;
        fileInfo->filename = mIsFuseMode ? mFuseTrimedFilename : mHostLogPath;
        fileInfo->offset = mLastFilePos - (int64_t)logBuffer.rawBuffer.size();
        fileInfo->len = (int64_t)logBuffer.rawBuffer.size();
        fileInfo->filePos = mLastFilePos;
        fileInfo->readPos = GetLastReadPos();
        fileInfo->fileSize = fileSize;
    }

    if (mIsFuseMode && logBuffer.rawBuffer.size() > 0)
        UlogfsHandler::GetInstance()->Sparse(logBuffer.fileInfo.get());

    if (mContainerStopped) {
        int32_t curTime = time(NULL);
        if (curTime - mContainerStoppedTime >= INT32_FLAG(logtail_alarm_interval)
            && curTime - mReadStoppedContainerAlarmTime >= INT32_FLAG(logtail_alarm_interval)) {
            mReadStoppedContainerAlarmTime = curTime;
            LOG_WARNING(sLogger,
                        ("read stopped container file", mHostLogPath)("stopped time", mContainerStoppedTime)(
                            "file size", fileSize)("read pos", mLastFilePos));
            LogtailAlarm::GetInstance()->SendAlarm(
                READ_STOPPED_CONTAINER_ALARM,
                string("path: ") + mHostLogPath + ", stopped time:" + ToString(mContainerStoppedTime)
                    + ", file size:" + ToString(fileSize) + ", now position:" + ToString(mLastFilePos),
                mProjectName,
                mCategory,
                mRegion);
        }
    }

    return moreData;
}

size_t LogFileReader::getNextReadSize(int64_t fileEnd, bool& fromCpt) {
    size_t readSize = static_cast<size_t>(fileEnd - mLastFilePos);
    bool allowMoreBufferSize = false;
    fromCpt = false;
    if (mEOOption && mEOOption->selectedCheckpoint->IsComplete()) {
        fromCpt = true;
        allowMoreBufferSize = true;
        auto& checkpoint = mEOOption->selectedCheckpoint->data;
        readSize = checkpoint.read_length();
        LOG_INFO(sLogger, ("read specified length", readSize)("offset", mLastFilePos));
    }
    if (readSize > BUFFER_SIZE && !allowMoreBufferSize) {
        readSize = BUFFER_SIZE;
    }
    return readSize;
}

void LogFileReader::setExactlyOnceCheckpointAfterRead(size_t readSize) {
    if (!mEOOption || readSize == 0) {
        return;
    }

    auto& cpt = mEOOption->selectedCheckpoint->data;
    cpt.set_read_offset(mLastFilePos);
    cpt.set_read_length(readSize);
}

void LogFileReader::ReadUTF8(LogBuffer& logBuffer, int64_t end, bool& moreData, bool allowRollback) {
    logBuffer.readOffset = mLastFilePos;
    bool fromCpt = false;
    size_t READ_BYTE = getNextReadSize(end, fromCpt);
    if (!READ_BYTE) {
        return;
    }
    const size_t lastCacheSize = mCache.size();
    if (READ_BYTE < lastCacheSize) {
        READ_BYTE = lastCacheSize; // this should not happen, just avoid READ_BYTE >= 0 theoratically
    }
    StringBuffer stringMemory = logBuffer.AllocateStringBuffer(READ_BYTE); // allocate modifiable buffer
    if (lastCacheSize) {
        READ_BYTE -= lastCacheSize; // reserve space to copy from cache if needed
    }
    TruncateInfo* truncateInfo = nullptr;
    int64_t lastReadPos = GetLastReadPos();
    size_t nbytes = READ_BYTE
        ? ReadFile(mLogFileOp, stringMemory.data + lastCacheSize, READ_BYTE, lastReadPos, &truncateInfo)
        : 0UL;
    char* stringBuffer = stringMemory.data;
    if (nbytes == 0 && (!lastCacheSize || allowRollback)) { // read nothing, if no cached data or allow rollback the
        // reader's state cannot be changed
        return;
    }
    if (lastCacheSize) {
        memcpy(stringBuffer, mCache.data(), lastCacheSize); // copy from cache
        nbytes += lastCacheSize;
    }
    // Ignore \n if last is force read
    if (stringBuffer[0] == '\n' && mLastForceRead) {
        ++stringBuffer;
        ++mLastFilePos;
        logBuffer.readOffset = mLastFilePos;
        --nbytes;
    }
    const size_t stringBufferLen = nbytes;
    logBuffer.truncateInfo.reset(truncateInfo);
    lastReadPos = mLastFilePos + nbytes; // this doesn't seem right when ulogfs is used and a hole is skipped
    LOG_DEBUG(sLogger, ("read bytes", nbytes)("last read pos", lastReadPos));
    moreData = (nbytes == BUFFER_SIZE);
    auto alignedBytes = nbytes;
    if (allowRollback) {
        alignedBytes = AlignLastCharacter(stringBuffer, nbytes);
    }
    if (allowRollback || mLogType == JSON_LOG) {
        int32_t rollbackLineFeedCount;
        nbytes = LastMatchedLine(stringBuffer, alignedBytes, rollbackLineFeedCount, allowRollback);
    }

    if (nbytes == 0) {
        if (moreData) { // excessively long line without '\n' or multiline begin or valid wchar
            nbytes = alignedBytes ? alignedBytes : BUFFER_SIZE;
            if (mLogType == JSON_LOG) {
                int32_t rollbackLineFeedCount;
                nbytes = LastMatchedLine(stringBuffer, nbytes, rollbackLineFeedCount, false);
            }
            LOG_WARNING(
                sLogger,
                ("Log is too long and forced to be split at offset: ", mLastFilePos + nbytes)("file: ", mHostLogPath)(
                    "inode: ", mDevInode.inode)("first 1024B log: ", logBuffer.rawBuffer.substr(0, 1024)));
            std::ostringstream oss;
            oss << "Log is too long and forced to be split at offset: " << ToString(mLastFilePos + nbytes)
                << " file: " << mHostLogPath << " inode: " << ToString(mDevInode.inode)
                << " first 1024B log: " << logBuffer.rawBuffer.substr(0, 1024) << std::endl;
            LogtailAlarm::GetInstance()->SendAlarm(SPLIT_LOG_FAIL_ALARM, oss.str(), mProjectName, mCategory, mRegion);
        } else {
            // line is not finished yet nor more data, put all data in cache
            mCache.assign(stringBuffer, stringBufferLen);
            return;
        }
    }
    if (nbytes < stringBufferLen) {
        // rollback happend, put rollbacked part in cache
        mCache.assign(stringBuffer + nbytes, stringBufferLen - nbytes);
    } else {
        mCache.clear();
    }
    // cache is sealed, nbytes should no change any more
    size_t stringLen = nbytes;
    if (stringBuffer[stringLen - 1] == '\n'
        || stringBuffer[stringLen - 1]
            == '\0') { // \0 is for json, such behavior make ilogtail not able to collect binary log
        --stringLen;
    }
    stringBuffer[stringLen] = '\0';

    if (!moreData && fromCpt && lastReadPos < end) {
        moreData = true;
    }
    logBuffer.rawBuffer = StringView(stringBuffer, stringLen); // set readable buffer
    logBuffer.readLength = nbytes;
    setExactlyOnceCheckpointAfterRead(nbytes);
    mLastFilePos += nbytes;

    mLastForceRead = !allowRollback;
    LOG_DEBUG(sLogger, ("read size", nbytes)("last file pos", mLastFilePos));
}

void LogFileReader::ReadGBK(LogBuffer& logBuffer, int64_t end, bool& moreData, bool allowRollback) {
    logBuffer.readOffset = mLastFilePos;
    bool fromCpt = false;
    size_t READ_BYTE = getNextReadSize(end, fromCpt);
    const size_t lastCacheSize = mCache.size();
    if (READ_BYTE < lastCacheSize) {
        READ_BYTE = lastCacheSize; // this should not happen, just avoid READ_BYTE >= 0 theoratically
    }
    std::unique_ptr<char[]> gbkMemory(new char[READ_BYTE + 1]);
    char* gbkBuffer = gbkMemory.get();
    if (lastCacheSize) {
        READ_BYTE -= lastCacheSize; // reserve space to copy from cache if needed
    }
    TruncateInfo* truncateInfo = nullptr;
    int64_t lastReadPos = GetLastReadPos();
    size_t readCharCount
        = READ_BYTE ? ReadFile(mLogFileOp, gbkBuffer + lastCacheSize, READ_BYTE, lastReadPos, &truncateInfo) : 0UL;
    if (readCharCount == 0 && (!lastCacheSize || allowRollback)) { // just keep last cache
        return;
    }
    if (lastCacheSize) {
        memcpy(gbkBuffer, mCache.data(), lastCacheSize); // copy from cache
        readCharCount += lastCacheSize;
    }
    // Ignore \n if last is force read
    if (gbkBuffer[0] == '\n' && mLastForceRead) {
        ++gbkBuffer;
        --readCharCount;
        ++mLastFilePos;
        logBuffer.readOffset = mLastFilePos;
    }
    logBuffer.truncateInfo.reset(truncateInfo);
    lastReadPos = mLastFilePos + readCharCount;
    const size_t originReadCount = readCharCount;
    moreData = (readCharCount == BUFFER_SIZE);
    bool logTooLongSplitFlag = false;
    auto alignedBytes = readCharCount;
    if (allowRollback) {
        alignedBytes = AlignLastCharacter(gbkBuffer, readCharCount);
    }
    if (alignedBytes == 0) {
        if (moreData) { // excessively long line without valid wchar
            logTooLongSplitFlag = true;
            alignedBytes = BUFFER_SIZE;
        } else {
            // line is not finished yet nor more data, put all data in cache
            mCache.assign(gbkBuffer, originReadCount);
            return;
        }
    }
    readCharCount = alignedBytes;
    gbkBuffer[readCharCount] = '\0';

    vector<long> lineFeedPos = {-1}; // elements point to the last char of each line
    for (long idx = 0; idx < long(readCharCount - 1); ++idx) {
        if (gbkBuffer[idx] == '\n')
            lineFeedPos.push_back(idx);
    }
    lineFeedPos.push_back(readCharCount - 1);

    size_t srcLength = readCharCount;
    size_t requiredLen
        = EncodingConverter::GetInstance()->ConvertGbk2Utf8(gbkBuffer, &srcLength, nullptr, 0, lineFeedPos);
    StringBuffer stringMemory = logBuffer.AllocateStringBuffer(requiredLen + 1);
    size_t resultCharCount = EncodingConverter::GetInstance()->ConvertGbk2Utf8(
        gbkBuffer, &srcLength, stringMemory.data, stringMemory.capacity, lineFeedPos);
    char* stringBuffer = stringMemory.data; // utf8 buffer
    if (resultCharCount == 0) {
        if (readCharCount < originReadCount) {
            // skip unconvertable part, put rollbacked part in cache
            mCache.assign(gbkBuffer + readCharCount, originReadCount - readCharCount);
        } else {
            mCache.clear();
        }
        mLastFilePos += readCharCount;
        logBuffer.readOffset = mLastFilePos;
        return;
    }
    int32_t rollbackLineFeedCount = 0;
    int32_t bakResultCharCount = resultCharCount;
    if (allowRollback || mLogType == JSON_LOG) {
        resultCharCount = LastMatchedLine(stringBuffer, resultCharCount, rollbackLineFeedCount, allowRollback);
    }
    if (resultCharCount == 0) {
        if (moreData) {
            resultCharCount = bakResultCharCount;
            rollbackLineFeedCount = 0;
            if (mLogType == JSON_LOG) {
                int32_t rollbackLineFeedCount;
                LastMatchedLine(stringBuffer, resultCharCount, rollbackLineFeedCount, false);
            }
            // Cannot get the split position here, so just mark a flag and send alarm later
            logTooLongSplitFlag = true;
        } else {
            // line is not finished yet nor more data, put all data in cache
            mCache.assign(gbkBuffer, originReadCount);
            return;
        }
    }

    int32_t lineFeedCount = lineFeedPos.size();
    if (rollbackLineFeedCount > 0 && lineFeedCount >= (1 + rollbackLineFeedCount)) {
        readCharCount -= lineFeedPos[lineFeedCount - 1] - lineFeedPos[lineFeedCount - 1 - rollbackLineFeedCount];
    }
    if (readCharCount < originReadCount) {
        // rollback happend, put rollbacked part in cache
        mCache.assign(gbkBuffer + readCharCount, originReadCount - readCharCount);
    } else {
        mCache.clear();
    }
    // cache is sealed, readCharCount should not change any more
    size_t stringLen = resultCharCount;
    if (stringBuffer[stringLen - 1] == '\n'
        || stringBuffer[stringLen - 1]
            == '\0') { // \0 is for json, such behavior make ilogtail not able to collect binary log
        --stringLen;
    }
    stringBuffer[stringLen] = '\0';
    if (!moreData && fromCpt && lastReadPos < end) {
        moreData = true;
    }
    logBuffer.rawBuffer = StringView(stringBuffer, stringLen);
    logBuffer.readLength = readCharCount;
    setExactlyOnceCheckpointAfterRead(readCharCount);
    mLastFilePos += readCharCount;
    if (logTooLongSplitFlag) {
        LOG_WARNING(sLogger,
                    ("Log is too long and forced to be split at offset: ", mLastFilePos)("file: ", mHostLogPath)(
                        "inode: ", mDevInode.inode)("first 1024B log: ", logBuffer.rawBuffer.substr(0, 1024)));
        std::ostringstream oss;
        oss << "Log is too long and forced to be split at offset: " << ToString(mLastFilePos)
            << " file: " << mHostLogPath << " inode: " << ToString(mDevInode.inode)
            << " first 1024B log: " << logBuffer.rawBuffer.substr(0, 1024) << std::endl;
        LogtailAlarm::GetInstance()->SendAlarm(SPLIT_LOG_FAIL_ALARM, oss.str(), mProjectName, mCategory, mRegion);
    }
    mLastForceRead = !allowRollback;
    LOG_DEBUG(sLogger,
              ("read gbk buffer, offset", mLastFilePos)("origin read", originReadCount)("at last read", readCharCount));
}

size_t
LogFileReader::ReadFile(LogFileOperator& op, void* buf, size_t size, int64_t& offset, TruncateInfo** truncateInfo) {
    if (buf == NULL || size == 0 || op.IsOpen() == false) {
        LOG_WARNING(sLogger, ("invalid param", ""));
        return 0;
    }

    int nbytes = 0;
    if (mIsFuseMode) {
        int64_t oriOffset = offset;
        nbytes = op.SkipHoleRead(buf, 1, size, &offset);
        if (nbytes < 0) {
            LOG_ERROR(sLogger,
                      ("SkipHoleRead fail to read log file",
                       mHostLogPath)("mLastFilePos", mLastFilePos)("size", size)("offset", offset));
            return 0;
        }
        if (oriOffset != offset && truncateInfo != NULL) {
            *truncateInfo = new TruncateInfo(oriOffset, offset);
            LOG_INFO(sLogger,
                     ("read fuse file with a hole, size",
                      offset - oriOffset)("filename", mHostLogPath)("dev", mDevInode.dev)("inode", mDevInode.inode));
            LogtailAlarm::GetInstance()->SendAlarm(
                FUSE_FILE_TRUNCATE_ALARM,
                string("read fuse file with a hole, size: ") + ToString(offset - oriOffset) + " filename: "
                    + mHostLogPath + " dev: " + ToString(mDevInode.dev) + " inode: " + ToString(mDevInode.inode),
                mProjectName,
                mCategory,
                mRegion);
        }
    } else {
        nbytes = op.Pread(buf, 1, size, offset);
        if (nbytes < 0) {
            LOG_ERROR(sLogger,
                      ("Pread fail to read log file", mHostLogPath)("mLastFilePos",
                                                                    mLastFilePos)("size", size)("offset", offset));
            return 0;
        }
    }

    *((char*)buf + nbytes) = '\0';
    return nbytes;
}

LogFileReader::FileCompareResult LogFileReader::CompareToFile(const string& filePath) {
    LogFileOperator logFileOp;
    logFileOp.Open(filePath.c_str(), mIsFuseMode);
    if (logFileOp.IsOpen() == false) {
        return FileCompareResult_Error;
    }

    fsutil::PathStat buf;
    if (0 == logFileOp.Stat(buf)) {
        auto devInode = buf.GetDevInode();
        if (devInode != mDevInode) {
            logFileOp.Close();
            return FileCompareResult_DevInodeChange;
        }

        char sigStr[1025];
        int readSize = logFileOp.Pread(sigStr, 1, 1024, 0);
        logFileOp.Close();
        if (readSize < 0)
            return FileCompareResult_Error;
        sigStr[readSize] = '\0';
        uint64_t sigHash = mLastFileSignatureHash;
        uint32_t sigSize = mLastFileSignatureSize;
        // If file size is 0 and filename is changed, we cannot judge if the inode is reused by signature,
        // so we just recreate the reader to avoid filename mismatch
        if (sigSize == 0 && filePath != mHostLogPath) {
            return FileCompareResult_SigChange;
        }
        bool sigSameRst = CheckAndUpdateSignature(string(sigStr), sigHash, sigSize);
        if (!sigSameRst) {
            return FileCompareResult_SigChange;
        }
        if ((int64_t)buf.GetFileSize() == mLastFilePos) {
            return FileCompareResult_SigSameSizeSame;
        } else {
            return FileCompareResult_SigSameSizeChange;
        }
    }
    logFileOp.Close();
    return FileCompareResult_Error;
}

/*
    Rollback from the end to ensure that the logs before are complete.
    Here are expected behaviours of this function:
    1. The logs remained must be complete.
    2. The logs rollbacked may be complete but we cannot confirm. Leave them to the next reading.
    3. The last line without '\n' is considered as unmatch. (even if it can match END regex)
    4. The '\n' at the end is considered as part of the multiline log.
    Examples:
    1. mLogBeginRegPtr != NULL
        1. begin\nxxx\nbegin\nxxx\n -> begin\nxxx\n
    2. mLogBeginRegPtr != NULL, mLogContinueRegPtr != NULL
        1. begin\ncontinue\nxxx\n -> begin\ncontinue\n
        2. begin\ncontinue\nbegin\n -> begin\ncontinue\n
        3. begin\ncontinue\nbegin\ncontinue\n -> begin\ncontinue\n
    3. mLogBeginRegPtr != NULL, mLogEndRegPtr != NULL
        1. begin\nxxx\nend\n -> begin\nxxx\nend
        2. begin\nxxx\nend\nbegin\nxxx\n -> begin\nxxx\nend
    4. mLogContinueRegPtr != NULL, mLogEndRegPtr != NULL
        1. continue\nend\n -> continue\nxxx\nend
        2. continue\nend\ncontinue\n -> continue\nxxx\nend
        3. continue\nend\ncontinue\nend\n -> continue\nxxx\nend
    5. mLogEndRegPtr != NULL
        1. xxx\nend\n -> xxx\nend
        1. xxx\nend\nxxx\n -> xxx\nend
*/
int32_t LogFileReader::LastMatchedLine(char* buffer, int32_t size, int32_t& rollbackLineFeedCount, bool allowRollback) {
    if (!allowRollback) {
        return size;
    }
    int endPs = size - 1; // buffer[size] = 0 , buffer[size-1] = '\n'
    rollbackLineFeedCount = 0;
    // Single line rollback
    if (!IsMultiLine()) {
        while (endPs >= 0) {
            if (buffer[endPs] == '\n') {
                if (endPs != size - 1) { // if last line dose not end with '\n', rollback
                    ++rollbackLineFeedCount;
                }
                return endPs + 1;
            }
            endPs--;
        }
        return 0;
    }
    // Multiline rollback
    int begPs = size - 2;
    std::string exception;
    while (begPs >= 0) {
        if (buffer[begPs] == '\n' || begPs == 0) {
            int lineBegin = begPs == 0 ? 0 : begPs + 1;
            if (mLogContinueRegPtr != NULL
                && BoostRegexMatch(buffer + lineBegin, endPs - lineBegin, *mLogContinueRegPtr, exception)) {
                ++rollbackLineFeedCount;
                endPs = begPs;
            } else if (mLogEndRegPtr != NULL
                       && BoostRegexMatch(buffer + lineBegin, endPs - lineBegin, *mLogEndRegPtr, exception)) {
                // Ensure the end line is complete
                if (buffer[endPs] == '\n') {
                    return endPs + 1;
                } else {
                    ++rollbackLineFeedCount;
                    endPs = begPs;
                }
            } else if (mLogBeginRegPtr != NULL
                       && BoostRegexMatch(buffer + lineBegin, endPs - lineBegin, *mLogBeginRegPtr, exception)) {
                ++rollbackLineFeedCount;
                // Keep all the buffer if rollback all
                return lineBegin;
            } else if (mLogContinueRegPtr != NULL) {
                // We can confirm the logs before are complete if continue is configured but no regex pattern can match.
                if (buffer[endPs] == '\n') {
                    return endPs + 1;
                } else {
                    // Keep all the buffer if rollback all
                    return lineBegin;
                }
            } else {
                ++rollbackLineFeedCount;
                endPs = begPs;
            }
        }
        begPs--;
    }
    return 0;
}

size_t LogFileReader::AlignLastCharacter(char* buffer, size_t size) {
    int n = 0;
    int endPs = size - 1;
    if (buffer[endPs] == '\n') {
        return size;
    }
    if (mFileEncoding == ENCODING_GBK) {
        // GB 18030 encoding rules:
        // 1. The number of byte for one character can be 1, 2, 4.
        // 2. 1 byte character: the top bit is 0.
        // 3. 2 bytes character: the 1st byte is between 0x81 and 0xFE; the 2nd byte is between 0x40 and 0xFE.
        // 4. 4 bytes character: the 1st and 3rd byte is between 0x81 and 0xFE; the 2nd and 4th byte are between 0x30
        // and 0x39. (not supported to align)

        // 1 byte character, 2nd byte of 2 bytes, 2nd or 4th byte of 4 bytes
        if ((buffer[endPs] & 0x80) == 0 || size == 1) {
            return size;
        }
        while (endPs >= 0 && (buffer[endPs] & 0x80)) {
            --endPs;
        }
        // whether characters >= 0x80 appear in pair
        if (((size - endPs - 1) & 1) == 0) {
            return size;
        }
        return size - 1;
    } else {
        // UTF8 encoding rules:
        // 1. For single byte character, the top bit is 0.
        // 2. For N (N > 1) bytes character, the top N bit of the first byte is 1. The first and second bits of the
        // following bytes are 10.
        while (endPs >= 0) {
            char ch = buffer[endPs];
            if ((ch & 0x80) == 0) { // 1 bytes character, 0x80 -> 10000000
                n = 1;
                break;
            } else if ((ch & 0xE0) == 0xC0) { // 2 bytes character, 0xE0 -> 11100000, 0xC0 -> 11000000
                n = 2;
                break;
            } else if ((ch & 0xF0) == 0xE0) { // 3 bytes character, 0xF0 -> 11110000, 0xE0 -> 11100000
                n = 3;
                break;
            } else if ((ch & 0xF8) == 0xF0) { // 4 bytes character, 0xF8 -> 11111000, 0xF0 -> 11110000
                n = 4;
                break;
            } else if ((ch & 0xFC) == 0xF8) { // 5 bytes character, 0xFC -> 11111100, 0xF8 -> 11111000
                n = 5;
                break;
            } else if ((ch & 0xFE) == 0xFC) { // 6 bytes character, 0xFE -> 11111110, 0xFC -> 11111100
                n = 6;
                break;
            }
            endPs--;
        }
        if (endPs - 1 + n >= (int)size) {
            return endPs;
        } else {
            return size;
        }
    }
    return 0;
}

std::unique_ptr<Event> LogFileReader::CreateFlushTimeoutEvent() {
    auto result = std::unique_ptr<Event>(new Event(mHostLogPathDir,
                                                   mHostLogPathFile,
                                                   EVENT_READER_FLUSH_TIMEOUT | EVENT_MODIFY,
                                                   -1,
                                                   0,
                                                   mDevInode.dev,
                                                   mDevInode.inode));
    result->SetLastFilePos(mLastFilePos);
    result->SetLastReadPos(GetLastReadPos());
    return result;
}

void LogFileReader::SetLogMultilinePolicy(const std::string& begReg,
                                          const std::string& conReg,
                                          const std::string& endReg) {
    if (mLogBeginRegPtr != NULL) {
        delete mLogBeginRegPtr;
        mLogBeginRegPtr = NULL;
    }
    if (begReg.empty() == false && begReg != ".*") {
        mLogBeginRegPtr = new boost::regex(begReg.c_str());
    }
    if (mLogContinueRegPtr != NULL) {
        delete mLogContinueRegPtr;
        mLogContinueRegPtr = NULL;
    }
    if (conReg.empty() == false && conReg != ".*") {
        mLogContinueRegPtr = new boost::regex(conReg.c_str());
    }
    if (mLogEndRegPtr != NULL) {
        delete mLogEndRegPtr;
        mLogEndRegPtr = NULL;
    }
    if (endReg.empty() == false && endReg != ".*") {
        mLogEndRegPtr = new boost::regex(endReg.c_str());
    }
}

LogFileReader::~LogFileReader() {
    if (mLogBeginRegPtr != NULL) {
        delete mLogBeginRegPtr;
        mLogBeginRegPtr = NULL;
    }
    if (mLogContinueRegPtr != NULL) {
        delete mLogContinueRegPtr;
        mLogContinueRegPtr = NULL;
    }
    if (mLogEndRegPtr != NULL) {
        delete mLogEndRegPtr;
        mLogEndRegPtr = NULL;
    }
    LOG_INFO(sLogger,
             ("destruct the corresponding log reader, project", mProjectName)("logstore", mCategory)(
                 "config", mConfigName)("log reader queue name", mHostLogPath)("file device", ToString(mDevInode.dev))(
                 "file inode", ToString(mDevInode.inode))("file signature", mLastFileSignatureHash)(
                 "file signature size", mLastFileSignatureSize)("file size", mLastFileSize)("last file position",
                                                                                            mLastFilePos));
    CloseFilePtr();

    // Mark GC so that corresponding resources can be released.
    // For config update, reader will be recreated, which will retrieve these
    //  resources back, then their GC flag will be removed.
    if (mEOOption) {
        static auto sQueueM = QueueManager::GetInstance();
        sQueueM->MarkGC(mProjectName,
                        mEOOption->primaryCheckpointKey + mEOOption->rangeCheckpointPtrs[0]->data.hash_key());

        static auto sCptM = CheckpointManagerV2::GetInstance();
        sCptM->MarkGC(mEOOption->primaryCheckpointKey);
    }
}

#ifdef APSARA_UNIT_TEST_MAIN
void LogFileReader::UpdateReaderManual() {
    if (mLogFileOp.IsOpen()) {
        mLogFileOp.Close();
    }
    mLogFileOp.Open(mHostLogPath.c_str(), mIsFuseMode);
    mDevInode = GetFileDevInode(mHostLogPath);
    mRealLogPath = mHostLogPath;
}
#endif

} // namespace logtail
