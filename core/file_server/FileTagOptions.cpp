/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "file_server/FileTagOptions.h"

#include "common/ParamExtractor.h"

namespace logtail {

bool FileTagOptions::Init(const Json::Value& config,
                          const PipelineContext& context,
                          const std::string& pluginType,
                          bool enableContainerDiscovery) {
    std::string errorMsg;

    // AppendingLogPositionMeta
    bool appendingLogPositionMeta = false;
    if (!GetOptionalBoolParam(config, "AppendingLogPositionMeta", appendingLogPositionMeta, errorMsg)) {
        PARAM_WARNING_DEFAULT(context.GetLogger(),
                              context.GetAlarm(),
                              errorMsg,
                              appendingLogPositionMeta,
                              pluginType,
                              context.GetConfigName(),
                              context.GetProjectName(),
                              context.GetLogstoreName(),
                              context.GetRegion());
    }

    // Tags
    const char* tagKey = "Tags";
    const Json::Value* tagConfig = config.find(tagKey, tagKey + strlen(tagKey));
    if (tagConfig) {
        if (!tagConfig->isObject()) {
            PARAM_WARNING_DEFAULT(context.GetLogger(),
                                  context.GetAlarm(),
                                  "param Tags is not of type object",
                                  "custom",
                                  pluginType,
                                  context.GetConfigName(),
                                  context.GetProjectName(),
                                  context.GetLogstoreName(),
                                  context.GetRegion());
            tagConfig = nullptr;
        }
    }

    // the priority of FileOffsetKey and FileInodeTagKey is higher than appendingLogPositionMeta
    if (config.isMember("FileOffsetKey") || (tagConfig && tagConfig->isMember("FileInodeTagKey"))) {
        parseDefaultNotAddTag(&config, "FileOffsetKey", TagKey::FILE_OFFSET_KEY, context, pluginType);
        parseDefaultNotAddTag(tagConfig, "FileInodeTagKey", TagKey::FILE_INODE_TAG_KEY, context, pluginType);
    } else if (appendingLogPositionMeta) {
        mFileTags[TagKey::FILE_OFFSET_KEY] = TagKeyDefaultValue[TagKey::FILE_OFFSET_KEY];
        mFileTags[TagKey::FILE_INODE_TAG_KEY] = TagKeyDefaultValue[TagKey::FILE_INODE_TAG_KEY];
    }

    parseDefaultAddTag(tagConfig, "FilePathTagKey", TagKey::FILE_PATH_TAG_KEY, context, pluginType);

    // ContainerDiscovery
    if (enableContainerDiscovery) {
        parseDefaultAddTag(tagConfig, "K8sNamespaceTagKey", TagKey::K8S_NAMESPACE_TAG_KEY, context, pluginType);
        parseDefaultAddTag(tagConfig, "K8sPodNameTagKey", TagKey::K8S_POD_NAME_TAG_KEY, context, pluginType);
        parseDefaultAddTag(tagConfig, "K8sPodUidTagKey", TagKey::K8S_POD_UID_TAG_KEY, context, pluginType);
        parseDefaultAddTag(tagConfig, "ContainerNameTagKey", TagKey::K8S_CONTAINER_NAME_TAG_KEY, context, pluginType);
        parseDefaultAddTag(tagConfig, "ContainerIpTagKey", TagKey::K8S_CONTAINER_IP_TAG_KEY, context, pluginType);
        parseDefaultAddTag(
            tagConfig, "ContainerImageNameTagKey", TagKey::K8S_CONTAINER_IMAGE_NAME_TAG_KEY, context, pluginType);

        parseDefaultAddTag(tagConfig, "ContainerNameTagKey", TagKey::CONTAINER_NAME_TAG_KEY, context, pluginType);
        parseDefaultAddTag(tagConfig, "ContainerIpTagKey", TagKey::CONTAINER_IP_TAG_KEY, context, pluginType);
        parseDefaultAddTag(
            tagConfig, "ContainerImageNameTagKey", TagKey::CONTAINER_IMAGE_NAME_TAG_KEY, context, pluginType);
    }

    return true;
}

StringView FileTagOptions::GetFileTagKeyName(TagKey key) const {
    auto it = mFileTags.find(key);
    if (it != mFileTags.end()) {
        // FileTagOption will not be deconstructed or changed before all event be sent
        return StringView(it->second.c_str(), it->second.size());
    }
    return StringView();
}

void FileTagOptions::parseDefaultAddTag(const Json::Value* config,
                                        const std::string& keyName,
                                        const TagKey& keyEnum,
                                        const PipelineContext& context,
                                        const std::string& pluginType) {
    std::string errorMsg;
    std::string key;
    if (config && config->isMember(keyName)) {
        if (!GetOptionalStringParam(*config, keyName, key, errorMsg)) {
            PARAM_WARNING_DEFAULT(context.GetLogger(),
                                  context.GetAlarm(),
                                  errorMsg,
                                  "custom",
                                  pluginType,
                                  context.GetConfigName(),
                                  context.GetProjectName(),
                                  context.GetLogstoreName(),
                                  context.GetRegion());
        } else if (!key.empty()) {
            if (key == DEFAULT_CONFIG_TAG_KEY_VALUE) {
                mFileTags[keyEnum] = TagKeyDefaultValue[keyEnum];
            } else {
                mFileTags[keyEnum] = key;
            }
        }
    } else {
        mFileTags[keyEnum] = TagKeyDefaultValue[keyEnum];
    }
}

void FileTagOptions::parseDefaultNotAddTag(const Json::Value* config,
                                           const std::string& keyName,
                                           const TagKey& keyEnum,
                                           const PipelineContext& context,
                                           const std::string& pluginType) {
    std::string errorMsg;
    std::string key;
    if (config && config->isMember(keyName)) {
        if (!GetOptionalStringParam(*config, keyName, key, errorMsg)) {
            PARAM_WARNING_DEFAULT(context.GetLogger(),
                                  context.GetAlarm(),
                                  errorMsg,
                                  "custom",
                                  pluginType,
                                  context.GetConfigName(),
                                  context.GetProjectName(),
                                  context.GetLogstoreName(),
                                  context.GetRegion());
        } else if (!key.empty()) {
            if (key == DEFAULT_CONFIG_TAG_KEY_VALUE) {
                mFileTags[keyEnum] = TagKeyDefaultValue[keyEnum];
            } else {
                mFileTags[keyEnum] = key;
            }
        }
    }
}

} // namespace logtail