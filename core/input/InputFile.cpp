#include "input/InputFile.h"

#include <filesystem>

#include "app_config/AppConfig.h"
#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"
#include "file/FileServer.h"
#include "InputFile.h"

using namespace std;

// Windows only has polling, give a bigger tail limit.
#if defined(__linux__)
DEFINE_FLAG_INT32(default_tail_limit_kb,
                  "when first open file, if offset little than this value, move offset to beginning, KB",
                  1024);
#elif defined(_MSC_VER)
DEFINE_FLAG_INT32(default_tail_limit_kb,
                  "when first open file, if offset little than this value, move offset to beginning, KB",
                  1024 * 50);
#endif
DEFINE_FLAG_INT32(delay_bytes_upperlimit,
                  "if (total_file_size - current_readed_size) exceed uppperlimit, send READ_LOG_DELAY_ALARM, bytes",
                  200 * 1024 * 1024);
DEFINE_FLAG_INT32(logreader_max_rotate_queue_size, "", 20);
DEFINE_FLAG_INT32(reader_close_unused_file_time, "second ", 60);
DEFINE_FLAG_INT32(search_checkpoint_default_dir_depth, "0 means only search current directory", 0);
DEFINE_FLAG_INT32(max_exactly_once_concurrency, "", 512);

namespace logtail {
InputFile::InputFile()
    : mTailSizeKB(static_cast<uint32_t>(INT32_FLAG(default_tail_limit_kb))),
      mReadDelayAlertThresholdBytes(static_cast<uint32_t>(INT32_FLAG(delay_bytes_upperlimit))),
      mRotatorQueueSize(static_cast<uint32_t>(INT32_FLAG(logreader_max_rotate_queue_size))),
      mCloseUnusedReaderIntervalSec(static_cast<uint32_t>(INT32_FLAG(reader_close_unused_file_time))),
      mMaxCheckpointDirSearchDepth(static_cast<uint32_t>(INT32_FLAG(search_checkpoint_default_dir_depth))) {
}

string InputFile::sName = "input_file";

bool InputFile::Init(const Table& config) {
    Json::Value config1;
    string errorMsg;

    // FilePaths + MaxDirSearchDepth
    if (!GetMandatoryListParam<string>(config1, "FilePaths", mFilePaths, errorMsg)) {
        PARAM_ERROR(sLogger, errorMsg, sName, mContext->GetConfigName());
    }
    if (mFilePaths.size() != 1) {
        PARAM_ERROR(sLogger, "param FilePaths has more than 1 element", sName, mContext->GetConfigName());
    }
    auto dirAndFile = GetDirAndFileNameFromPath(mFilePaths[0]);
    mBasePath = dirAndFile.first, mFilePattern = dirAndFile.second;
    if (mBasePath.empty() || mFilePattern.empty()) {
        PARAM_ERROR(sLogger, "param FilePaths[0] is invalid", sName, mContext->GetConfigName());
    }
    size_t len = mBasePath.size();
    if (len > 2 && mBasePath[len - 1] == '*' && mBasePath[len - 2] == '*'
        && mBasePath[len - 3] == filesystem::path::preferred_separator) {
        if (len == 3) {
            // for parent path like /**, we should maintain root dir, i.e., /
            mBasePath = mBasePath.substr(0, len - 2);
        } else {
            mBasePath = mBasePath.substr(0, len - 3);
        }
        // MaxDirSearchDepth is only valid when parent path ends with **
        if (!GetOptionalIntParam(config1, "MaxDirSearchDepth", mMaxDirSearchDepth, errorMsg)) {
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
        }
    }
    ParseWildcardPath();

    // ExcludeFilePaths
    if (!GetOptionalListParam<string>(config1, "ExcludeFilePaths", mExcludeFilePaths, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
    }
    for (size_t i = 0; i < mExcludeFilePaths.size(); ++i) {
        if (!filesystem::path(mExcludeFilePaths[i]).is_absolute()) {
            PARAM_WARNING_IGNORE(
                sLogger, "ExcludeFilePaths[" + ToString(i) + "] is not absolute", sName, mContext->GetConfigName());
            continue;
        }
        bool isMultipleLevelWildcard = mExcludeFilePaths[i].find("**") != std::string::npos;
        if (isMultipleLevelWildcard) {
            mMLFilePathBlacklist.push_back(mExcludeFilePaths[i]);
        } else {
            mFilePathBlacklist.push_back(mExcludeFilePaths[i]);
        }
    }

    // ExcludeFiles
    if (!GetOptionalListParam<string>(config1, "ExcludeFiles", mExcludeFiles, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
    }
    for (size_t i = 0; i < mExcludeFiles.size(); ++i) {
        if (mExcludeFiles[i].find(filesystem::path::preferred_separator) != std::string::npos) {
            PARAM_WARNING_IGNORE(
                sLogger, "ExcludeFiles[" + ToString(i) + "] contains path separator", sName, mContext->GetConfigName());
            continue;
        }
        mFileNameBlacklist.push_back(mExcludeFiles[i]);
    }

    // ExcludeDirs
    if (!GetOptionalListParam<string>(config1, "ExcludeDirs", mExcludeDirs, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
    }
    for (size_t i = 0; i < mExcludeDirs.size(); ++i) {
        if (!filesystem::path(mExcludeDirs[i]).is_absolute()) {
            PARAM_WARNING_IGNORE(
                sLogger, "ExcludeDirs[" + ToString(i) + "] is not absolute", sName, mContext->GetConfigName());
            continue;
        }
        bool isMultipleLevelWildcard = mExcludeDirs[i].find("**") != std::string::npos;
        if (isMultipleLevelWildcard) {
            mMLWildcardDirPathBlacklist.push_back(mExcludeDirs[i]);
            continue;
        }
        bool isWildcardPath
            = mExcludeDirs[i].find("*") != std::string::npos || mExcludeDirs[i].find("?") != std::string::npos;
        if (isWildcardPath) {
            mWildcardDirPathBlacklist.push_back(mExcludeDirs[i]);
        } else {
            mDirPathBlacklist.push_back(mExcludeDirs[i]);
        }
    }
    if (!mDirPathBlacklist.empty() || !mWildcardDirPathBlacklist.empty() || !mMLWildcardDirPathBlacklist.empty()
        || !mMLFilePathBlacklist.empty() || !mFileNameBlacklist.empty() || !mFilePathBlacklist.empty()) {
        mHasBlacklist = true;
    }

    // TailingAllMatchedFiles
    if (!GetOptionalBoolParam(config1, "TailingAllMatchedFiles", mTailingAllMatchedFiles, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }

    // FileEncoding
    string encoding;
    if (!GetOptionalStringParam(config1, "FileEncoding", encoding, errorMsg)) {
        PARAM_ERROR(sLogger, errorMsg, sName, mContext->GetConfigName());
    }
    encoding = ToLowerCaseString(encoding);
    if (encoding == "gbk") {
        mFileEncoding = Encoding::GBK;
    } else if (encoding != "utf8") {
        PARAM_ERROR(sLogger, "param FileEncoding is not valid", sName, mContext->GetConfigName());
    }

    // TailSizeKB
    uint32_t tailSize = 0;
    if (!GetOptionalUIntParam(config1, "TailSizeKB", tailSize, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, INT32_FLAG(default_tail_limit_kb), sName, mContext->GetConfigName());
    }
    if (tailSize <= 100 * 1024 * 1024) {
        mTailSizeKB = tailSize;
    } else {
        PARAM_WARNING_DEFAULT(sLogger,
                              "param TailSizeKB is larger than 104857600",
                              INT32_FLAG(default_tail_limit_kb),
                              sName,
                              mContext->GetConfigName());
    }

    // Multiline
    const char* key = "Multiline";
    const Json::Value* itr = config1.find(key, key + strlen(key));
    if (itr && itr->isObject()) {
        // Mode
        string mode;
        if (!GetOptionalStringParam(*itr, "Multiline.Mode", mode, errorMsg)) {
            mode = "custom";
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, "custom", sName, mContext->GetConfigName());
        }
        if (mode == "JSON") {
            mMultiline.mMode = Multiline::Mode::JSON;
        } else if (mode != "custom") {
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, "custom", sName, mContext->GetConfigName());
        }

        // StartPattern
        if (mMultiline.mMode == Multiline::Mode::CUSTOM) {
            if (!GetMandatoryStringParam(*itr, "Multiline.StartPattern", mMultiline.mStartPattern, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }
            if (!IsRegexValid(mMultiline.mStartPattern)) {
                mMultiline.mStartPattern.clear();
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }
        }
    }

    // EnableContainerDiscovery
    if (!GetOptionalBoolParam(config1, "EnableContainerDiscovery", mEnableContainerDiscovery, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }
    if (mEnableContainerDiscovery && !AppConfig::GetInstance()->IsPurageContainerMode()) {
        PARAM_ERROR(sLogger,
                    "iLogtail is not in container, but container discovery is required",
                    sName,
                    mContext->GetConfigName());
    }

    if (mEnableContainerDiscovery) {
        // ContainerFilters
        const char* key = "ContainerFilters";
        const Json::Value* itr = config1.find(key, key + strlen(key));
        if (itr && itr->isObject()) {
            // K8sNamespaceRegex
            if (!GetOptionalStringParam(
                    *itr, "ContainerFilters.K8sNamespaceRegex", mContainerFilters.mK8sNamespaceRegex, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // K8sPodRegex
            if (!GetOptionalStringParam(
                    *itr, "ContainerFilters.K8sPodRegex", mContainerFilters.mK8sPodRegex, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // IncludeK8sLabel
            if (!GetOptionalMapParam(
                    *itr, "ContainerFilters.IncludeK8sLabel", mContainerFilters.mIncludeK8sLabel, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // ExcludeK8sLabel
            if (!GetOptionalMapParam(
                    *itr, "ContainerFilters.ExcludeK8sLabel", mContainerFilters.mExcludeK8sLabel, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // K8sContainerRegex
            if (!GetOptionalStringParam(
                    *itr, "ContainerFilters.K8sContainerRegex", mContainerFilters.mK8sContainerRegex, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // IncludeEnv
            if (!GetOptionalMapParam(*itr, "ContainerFilters.IncludeEnv", mContainerFilters.mIncludeEnv, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // ExcludeEnv
            if (!GetOptionalMapParam(*itr, "ContainerFilters.ExcludeEnv", mContainerFilters.mExcludeEnv, errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // IncludeContainerLabel
            if (!GetOptionalMapParam(*itr,
                                     "ContainerFilters.IncludeContainerLabel",
                                     mContainerFilters.mIncludeContainerLabel,
                                     errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }

            // ExcludeContainerLabel
            if (!GetOptionalMapParam(*itr,
                                     "ContainerFilters.ExcludeContainerLabel",
                                     mContainerFilters.mExcludeContainerLabel,
                                     errorMsg)) {
                PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
            }
        }

        // ExternalK8sLabelTag
        if (!GetOptionalMapParam(config1, "ExternalK8sLabelTag", mExternalK8sLabelTag, errorMsg)) {
            PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
        }

        // ExternalEnvTag
        if (!GetOptionalMapParam(config1, "ExternalEnvTag", mExternalEnvTag, errorMsg)) {
            PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
        }

        // CollectingContainersMeta
        if (!GetOptionalBoolParam(config1, "CollectingContainersMeta", mCollectingContainersMeta, errorMsg)) {
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
        }
    }

    // AppendingLogPositionMeta
    if (!GetOptionalBoolParam(config1, "AppendingLogPositionMeta", mAppendingLogPositionMeta, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }

    // PreservedDirDepth
    if (!GetOptionalIntParam(config1, "PreservedDirDepth", mPreservedDirDepth, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, -1, sName, mContext->GetConfigName());
    }

    // ReadDelaySkipThresholdBytes
    if (!GetOptionalUIntParam(config1, "ReadDelaySkipThresholdBytes", mReadDelaySkipThresholdBytes, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
    }

    // ReadDelaySkipThresholdBytes
    if (!GetOptionalUIntParam(config1, "ReadDelaySkipThresholdBytes", mReadDelaySkipThresholdBytes, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, INT32_FLAG(delay_bytes_upperlimit), sName, mContext->GetConfigName());
    }

    // RotatorQueueSize
    if (!GetOptionalUIntParam(config1, "RotatorQueueSize", mRotatorQueueSize, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            sLogger, errorMsg, INT32_FLAG(logreader_max_rotate_queue_size), sName, mContext->GetConfigName());
    }

    // CloseUnusedReaderIntervalSec
    if (!GetOptionalUIntParam(config1, "CloseUnusedReaderIntervalSec", mCloseUnusedReaderIntervalSec, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            sLogger, errorMsg, INT32_FLAG(reader_close_unused_file_time), sName, mContext->GetConfigName());
    }

    // AllowingCollectingFilesInRootDir
    if (!GetOptionalBoolParam(
            config1, "AllowingCollectingFilesInRootDir", mAllowingCollectingFilesInRootDir, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }
    if (mAllowingCollectingFilesInRootDir) {
        BOOL_FLAG(enable_root_path_collection) = mAllowingCollectingFilesInRootDir;
    }

    // AllowingIncludedByMultiConfigs
    if (!GetOptionalBoolParam(config1, "AllowingIncludedByMultiConfigs", mAllowingIncludedByMultiConfigs, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }

    // MaxCheckpointDirSearchDepth
    if (!GetOptionalUIntParam(config1, "MaxCheckpointDirSearchDepth", mMaxCheckpointDirSearchDepth, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
    }

    // ExactlyOnceConcurrency (param is unintentionally named as EnableExactlyOnce, which should be deprecated in the
    // future)
    if (!GetOptionalUIntParam(config1, "EnableExactlyOnce", mExactlyOnceConcurrency, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
    }
    if (mExactlyOnceConcurrency > INT32_FLAG(max_exactly_once_concurrency)) {
        mExactlyOnceConcurrency = 0;
        PARAM_WARNING_DEFAULT(
            sLogger, "param EnableExactlyOnce is larger than 512", false, sName, mContext->GetConfigName());
    }

    return true;
}

bool InputFile::Start() {
    mContainerInfos = FileServer::GetInstance()->GetAndRemoveContainerInfo(mContext->GetPipeline().lock()->Name());
    FileServer::GetInstance()->AddPipeline(mContext->GetPipeline().lock());
    return true;
}

bool InputFile::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        FileServer::GetInstance()->SaveContainerInfo(mContext->GetPipeline().lock()->Name(), mContainerInfos);
    }
    FileServer::GetInstance()->RemovePipeline(mContext->GetPipeline().lock());
    return true;
}

pair<string, string> InputFile::GetDirAndFileNameFromPath(const string& filePath) {
    filesystem::path path(filePath);
    if (path.is_relative()) {
        error_code ec;
        path = filesystem::absolute(path, ec);
    }
    path = path.lexically_normal();
    return make_pair(path.parent_path().string(), path.filename().string());
}

void InputFile::ParseWildcardPath() {
    mWildcardPaths.clear();
    mConstWildcardPaths.clear();
    mWildcardDepth = 0;
    if (mBasePath.size() == 0)
        return;
    size_t posA = mBasePath.find('*', 0);
    size_t posB = mBasePath.find('?', 0);
    size_t pos;
    if (posA == std::string::npos) {
        if (posB == std::string::npos)
            return;
        else
            pos = posB;
    } else {
        if (posB == std::string::npos)
            pos = posA;
        else
            pos = std::min(posA, posB);
    }
    if (pos == 0)
        return;
    pos = mBasePath.rfind(filesystem::path::preferred_separator, pos);
    if (pos == std::string::npos)
        return;

        // Check if there is only one path separator, for Windows, the first path
        // separator is next to the first ':'.
#if defined(__linux__)
    if (pos == 0)
#elif defined(_MSC_VER)
    if (pos - 1 == mBasePath.find(':'))
#endif
    {
        mWildcardPaths.push_back(mBasePath.substr(0, pos + 1));
    } else
        mWildcardPaths.push_back(mBasePath.substr(0, pos));
    while (true) {
        size_t nextPos = mBasePath.find(filesystem::path::preferred_separator, pos + 1);
        if (nextPos == std::string::npos)
            break;
        mWildcardPaths.push_back(mBasePath.substr(0, nextPos));
        std::string dirName = mBasePath.substr(pos + 1, nextPos - pos - 1);
        LOG_DEBUG(sLogger, ("wildcard paths", mWildcardPaths[mWildcardPaths.size() - 1])("dir name", dirName));
        if (dirName.find('?') == std::string::npos && dirName.find('*') == std::string::npos) {
            mConstWildcardPaths.push_back(dirName);
        } else {
            mConstWildcardPaths.push_back("");
        }
        pos = nextPos;
    }
    mWildcardPaths.push_back(mBasePath);
    if (pos < mBasePath.size()) {
        std::string dirName = mBasePath.substr(pos + 1);
        if (dirName.find('?') == std::string::npos && dirName.find('*') == std::string::npos) {
            mConstWildcardPaths.push_back(dirName);
        } else {
            mConstWildcardPaths.push_back("");
        }
    }

    for (size_t i = 0; i < mBasePath.size(); ++i) {
        if (filesystem::path::preferred_separator == mBasePath[i])
            ++mWildcardDepth;
    }
}
} // namespace logtail