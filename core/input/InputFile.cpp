#include "input/InputFile.h"

#include <filesystem>

#include "app_config/AppConfig.h"
#include "config_manager/ConfigManager.h"
#include "common/JsonUtil.h"
#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"
// #include "file/FileServer.h"
#include "pipeline/Pipeline.h"

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
DEFINE_FLAG_INT32(default_reader_flush_timeout, "", 5);
DEFINE_FLAG_INT32(default_plugin_log_queue_size, "", 10);

namespace logtail {
const string InputFile::sName = "input_file";

InputFile::InputFile()
    : mTailSizeKB(static_cast<uint32_t>(INT32_FLAG(default_tail_limit_kb))),
      mFlushTimeoutSecs(static_cast<uint32_t>(INT32_FLAG(default_reader_flush_timeout))),
      mReadDelayAlertThresholdBytes(static_cast<uint32_t>(INT32_FLAG(delay_bytes_upperlimit))),
      mRotatorQueueSize(static_cast<uint32_t>(INT32_FLAG(logreader_max_rotate_queue_size))),
      mCloseUnusedReaderIntervalSec(static_cast<uint32_t>(INT32_FLAG(reader_close_unused_file_time))),
      mMaxCheckpointDirSearchDepth(static_cast<uint32_t>(INT32_FLAG(search_checkpoint_default_dir_depth))) {
}

bool InputFile::Init(const Json::Value& config) {
    string errorMsg;

    // FilePaths + MaxDirSearchDepth
    if (!GetMandatoryListParam<string>(config, "FilePaths", mFilePaths, errorMsg)) {
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
        if (!GetOptionalIntParam(config, "MaxDirSearchDepth", mMaxDirSearchDepth, errorMsg)) {
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
        }
    }
    ParseWildcardPath();

    // ExcludeFilePaths
    if (!GetOptionalListParam<string>(config, "ExcludeFilePaths", mExcludeFilePaths, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
    } else {
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
    }

    // ExcludeFiles
    if (!GetOptionalListParam<string>(config, "ExcludeFiles", mExcludeFiles, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
    } else {
        for (size_t i = 0; i < mExcludeFiles.size(); ++i) {
            if (mExcludeFiles[i].find(filesystem::path::preferred_separator) != std::string::npos) {
                PARAM_WARNING_IGNORE(sLogger,
                                     "ExcludeFiles[" + ToString(i) + "] contains path separator",
                                     sName,
                                     mContext->GetConfigName());
                continue;
            }
            mFileNameBlacklist.push_back(mExcludeFiles[i]);
        }
    }

    // ExcludeDirs
    if (!GetOptionalListParam<string>(config, "ExcludeDirs", mExcludeDirs, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
    } else {
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
    }
    if (!mDirPathBlacklist.empty() || !mWildcardDirPathBlacklist.empty() || !mMLWildcardDirPathBlacklist.empty()
        || !mMLFilePathBlacklist.empty() || !mFileNameBlacklist.empty() || !mFilePathBlacklist.empty()) {
        mHasBlacklist = true;
    }

    // TailingAllMatchedFiles
    if (!GetOptionalBoolParam(config, "TailingAllMatchedFiles", mTailingAllMatchedFiles, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }

    // FileEncoding
    string encoding;
    if (!GetOptionalStringParam(config, "FileEncoding", encoding, errorMsg)) {
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
    if (!GetOptionalUIntParam(config, "TailSizeKB", tailSize, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, INT32_FLAG(default_tail_limit_kb), sName, mContext->GetConfigName());
    } else if (tailSize > 100 * 1024 * 1024) {
        PARAM_WARNING_DEFAULT(sLogger,
                              "param TailSizeKB is larger than 104857600",
                              INT32_FLAG(default_tail_limit_kb),
                              sName,
                              mContext->GetConfigName());
    } else {
        mTailSizeKB = tailSize;
    }

    // Multiline
    const char* key = "Multiline";
    const Json::Value* itr = config.find(key, key + strlen(key));
    if (itr) {
        if (!itr->isObject()) {
            PARAM_WARNING_IGNORE(sLogger, "param Multiline is not of type object", sName, mContext->GetConfigName());
        } else {
            // Mode
            string mode;
            if (!GetOptionalStringParam(*itr, "Multiline.Mode", mode, errorMsg)) {
                PARAM_WARNING_DEFAULT(sLogger, errorMsg, "custom", sName, mContext->GetConfigName());
            } else if (mode == "JSON") {
                mMultiline.mMode = Multiline::Mode::JSON;
            } else if (mode != "custom") {
                PARAM_WARNING_DEFAULT(sLogger, errorMsg, "custom", sName, mContext->GetConfigName());
            }

            if (mMultiline.mMode == Multiline::Mode::CUSTOM) {
                // StartPattern
                string pattern;
                if (!GetOptionalStringParam(*itr, "Multiline.StartPattern", pattern, errorMsg)) {
                    PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
                } else if (!IsRegexValid(pattern)) {
                    PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
                } else {
                    mMultiline.mStartPattern = pattern;
                }

                // ContinuePattern
                if (!GetOptionalStringParam(*itr, "Multiline.ContinuePattern", pattern, errorMsg)) {
                    PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
                } else if (!IsRegexValid(pattern)) {
                    PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
                } else {
                    mMultiline.mContinuePattern = pattern;
                }

                // EndPattern
                if (!GetOptionalStringParam(*itr, "Multiline.EndPattern", pattern, errorMsg)) {
                    PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
                } else if (!IsRegexValid(pattern)) {
                    PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
                } else {
                    mMultiline.mEndPattern = pattern;
                }
            }
        }
    }

    // EnableContainerDiscovery
    if (!GetOptionalBoolParam(config, "EnableContainerDiscovery", mEnableContainerDiscovery, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    } else if (mEnableContainerDiscovery && !AppConfig::GetInstance()->IsPurageContainerMode()) {
        PARAM_ERROR(sLogger,
                    "iLogtail is not in container, but container discovery is required",
                    sName,
                    mContext->GetConfigName());
    }

    if (mEnableContainerDiscovery) {
        // ContainerFilters
        const char* key = "ContainerFilters";
        const Json::Value* itr = config.find(key, key + strlen(key));
        if (itr) {
            if (!itr->isObject()) {
                PARAM_WARNING_IGNORE(
                    sLogger, "param ContainerFilters is not of type object", sName, mContext->GetConfigName());
            } else {
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
                if (!GetOptionalMapParam(
                        *itr, "ContainerFilters.IncludeEnv", mContainerFilters.mIncludeEnv, errorMsg)) {
                    PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
                }

                // ExcludeEnv
                if (!GetOptionalMapParam(
                        *itr, "ContainerFilters.ExcludeEnv", mContainerFilters.mExcludeEnv, errorMsg)) {
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
        }

        // ExternalK8sLabelTag
        if (!GetOptionalMapParam(config, "ExternalK8sLabelTag", mExternalK8sLabelTag, errorMsg)) {
            PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
        }

        // ExternalEnvTag
        if (!GetOptionalMapParam(config, "ExternalEnvTag", mExternalEnvTag, errorMsg)) {
            PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
        }

        // CollectingContainersMeta
        if (!GetOptionalBoolParam(config, "CollectingContainersMeta", mCollectingContainersMeta, errorMsg)) {
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
        }

        GenerateContainerMetaFetchingGoPipeline();

        // 过渡使用
        auto allContainers = ConfigManager::GetInstance()->GetAllContainerInfo();
        auto iter = allContainers.find(mContext->GetConfigName());
        if (iter != allContainers.end()) {
            mContainerInfos = iter->second;
            allContainers.erase(iter);
        }
    }

    // AppendingLogPositionMeta
    if (!GetOptionalBoolParam(config, "AppendingLogPositionMeta", mAppendingLogPositionMeta, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }

    // PreservedDirDepth
    if (!GetOptionalIntParam(config, "PreservedDirDepth", mPreservedDirDepth, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, -1, sName, mContext->GetConfigName());
    }

    // FlushTimeoutSecs
    if (!GetOptionalUIntParam(*itr, "FlushTimeoutSecs", mFlushTimeoutSecs, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            sLogger, errorMsg, INT32_FLAG(default_reader_flush_timeout), sName, mContext->GetConfigName());
    }

    // ReadDelaySkipThresholdBytes
    if (!GetOptionalUIntParam(config, "ReadDelaySkipThresholdBytes", mReadDelaySkipThresholdBytes, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
    }

    // ReadDelayAlertThresholdBytes
    if (!GetOptionalUIntParam(config, "ReadDelayAlertThresholdBytes", mReadDelayAlertThresholdBytes, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, INT32_FLAG(delay_bytes_upperlimit), sName, mContext->GetConfigName());
    }

    // RotatorQueueSize
    if (!GetOptionalUIntParam(config, "RotatorQueueSize", mRotatorQueueSize, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            sLogger, errorMsg, INT32_FLAG(logreader_max_rotate_queue_size), sName, mContext->GetConfigName());
    }

    // CloseUnusedReaderIntervalSec
    if (!GetOptionalUIntParam(config, "CloseUnusedReaderIntervalSec", mCloseUnusedReaderIntervalSec, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            sLogger, errorMsg, INT32_FLAG(reader_close_unused_file_time), sName, mContext->GetConfigName());
    }

    // AllowingCollectingFilesInRootDir
    if (!GetOptionalBoolParam(
            config, "AllowingCollectingFilesInRootDir", mAllowingCollectingFilesInRootDir, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    } else if (mAllowingCollectingFilesInRootDir) {
        BOOL_FLAG(enable_root_path_collection) = mAllowingCollectingFilesInRootDir;
    }

    // AllowingIncludedByMultiConfigs
    if (!GetOptionalBoolParam(config, "AllowingIncludedByMultiConfigs", mAllowingIncludedByMultiConfigs, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, sName, mContext->GetConfigName());
    }

    // MaxCheckpointDirSearchDepth
    if (!GetOptionalUIntParam(config, "MaxCheckpointDirSearchDepth", mMaxCheckpointDirSearchDepth, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
    }

    // ExactlyOnceConcurrency (param is unintentionally named as EnableExactlyOnce, which should be deprecated in the
    // future)
    uint32_t exactlyOnceConcurrency;
    if (!GetOptionalUIntParam(config, "EnableExactlyOnce", exactlyOnceConcurrency, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
    } else if (exactlyOnceConcurrency > INT32_FLAG(max_exactly_once_concurrency)) {
        PARAM_WARNING_DEFAULT(sLogger,
                              "param EnableExactlyOnce is larger than 512",
                              INT32_FLAG(max_exactly_once_concurrency),
                              sName,
                              mContext->GetConfigName());
    } else {
        mExactlyOnceConcurrency = exactlyOnceConcurrency;
    }

    return true;
}

bool InputFile::Start() {
    // mContainerInfos = FileServer::GetInstance()->GetAndRemoveContainerInfo(mContext->GetPipeline().lock()->Name());
    // FileServer::GetInstance()->AddPipeline(mContext->GetPipeline().lock());
    return true;
}

bool InputFile::Stop(bool isPipelineRemoving) {
    // if (!isPipelineRemoving) {
    //     FileServer::GetInstance()->SaveContainerInfo(mContext->GetPipeline().lock()->Name(), mContainerInfos);
    // }
    // FileServer::GetInstance()->RemovePipeline(mContext->GetPipeline().lock());
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

void InputFile::GenerateContainerMetaFetchingGoPipeline() const {
    Json::Value plugin(Json::objectValue), detail(Json::objectValue), object(Json::objectValue);
    auto ConvertMapToJsonObj = [&](const char* key, const unordered_map<string, string>& map) {
        if (!map.empty()) {
            object.clear();
            for (const auto& item : map) {
                object[item.first] = Json::Value(item.second);
            }
            detail[key] = object;
        }
    };

    if (!mWildcardPaths.empty()) {
        detail["LogPath"] = Json::Value(mWildcardPaths[0]);
        detail["MaxDepth"] = Json::Value(static_cast<int32_t>(mWildcardPaths.size()) + mMaxDirSearchDepth - 1);
    } else {
        detail["LogPath"] = Json::Value(mBasePath);
        detail["MaxDepth"] = Json::Value(mMaxDirSearchDepth);
    }
    detail["FileParttern"] = Json::Value(mFilePattern);
    if (!mContainerFilters.mK8sNamespaceRegex.empty()) {
        detail["K8sNamespaceRegex"] = Json::Value(mContainerFilters.mK8sNamespaceRegex);
    }
    if (!mContainerFilters.mK8sPodRegex.empty()) {
        detail["K8sPodRegex"] = Json::Value(mContainerFilters.mK8sPodRegex);
    }
    if (!mContainerFilters.mK8sContainerRegex.empty()) {
        detail["K8sContainerRegex"] = Json::Value(mContainerFilters.mK8sContainerRegex);
    }
    ConvertMapToJsonObj("IncludeK8sLabel", mContainerFilters.mIncludeK8sLabel);
    ConvertMapToJsonObj("ExcludeK8sLabel", mContainerFilters.mExcludeK8sLabel);
    ConvertMapToJsonObj("IncludeEnv", mContainerFilters.mIncludeEnv);
    ConvertMapToJsonObj("ExcludeEnv", mContainerFilters.mExcludeEnv);
    ConvertMapToJsonObj("IncludeContainerLabel", mContainerFilters.mIncludeContainerLabel);
    ConvertMapToJsonObj("ExcludeContainerLabel", mContainerFilters.mExcludeContainerLabel);
    ConvertMapToJsonObj("ExternalK8sLabelTag", mExternalK8sLabelTag);
    ConvertMapToJsonObj("ExternalEnvTag", mExternalEnvTag);
    if (mCollectingContainersMeta) {
        detail["CollectingContainersMeta"] = Json::Value(mCollectingContainersMeta);
    }
    plugin["type"] = Json::Value("metric_docker_file");
    plugin["detail"] = detail;
    Json::Value& inputs = mContext->GetPipeline().GetGoPipelineWithInput()["inputs"];
    inputs.append(plugin);

    Json::Value& global = mContext->GetPipeline().GetGoPipelineWithInput()["global"];
    SetNotFoundJsonMember(global, "DefaultLogQueueSize", INT32_FLAG(default_plugin_log_queue_size));
    SetNotFoundJsonMember(global, "AlwaysOnline", true);
}

} // namespace logtail