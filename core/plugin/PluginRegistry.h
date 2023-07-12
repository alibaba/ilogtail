/*
 * Copyright 2022 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <memory>
#include <string>
#include <set>
#include <unordered_map>
#include "plugin/PluginInstance.h"
#include "plugin/ProcessorInstance.h"
#include "common/DynamicLibHelper.h"

struct processor_interface_t;

namespace logtail {

class PluginRegistry {
public:
    static PluginRegistry* GetInstance();
    // 加载所有插件
    void LoadPlugins();

    // 卸载所有插件
    void UnloadPlugins();

    // InputInstance* CreateInput(const std::string& name, const std::string& pluginId);
    ProcessorInstance* CreateProcessor(const std::string& name, const std::string& pluginId);
    // FlusherInstance* CreateFlusher(const std::string& name, const std::string& pluginId);

private:
    enum PluginCat { INPUT_PLUGIN, PROCESSOR_PLUGIN, FLUSHER_PLUGIN };

    void LoadStaticPlugins();
    void LoadDynamicPlugins(const std::set<std::string>& plugins);
    // void RegisterInputCreator(PluginCreatorInterface* registry);
    void RegisterProcessorCreator(PluginCreatorInterface* registry);
    // void RegisterFlusherCreator(PluginCreatorInterface* registry);
    void UnregisterCreator(PluginCreatorInterface* node);
    PluginCreatorInterface* LoadProcessorPlugin(DynamicLibLoader& loader, const std::string pluginName);
    void RegisterCreator(PluginCat cat, PluginCreatorInterface* registry);
    PluginInstance* Create(PluginCat cat, const std::string& name, const std::string& pluginId);

    struct PluginKey {
        PluginCat cat;
        std::string name;
        PluginKey(PluginCat cat, const std::string& name) : cat(cat), name(name) {}
        bool operator==(const PluginKey& other) const { return cat == other.cat && name == other.name; }
    };

    struct PluginKeyHash {
        size_t operator()(const PluginKey& obj) const {
            return std::hash<int>()(obj.cat) ^ std::hash<std::string>()(obj.name);
        }
    };
    std::unordered_map<PluginKey, std::shared_ptr<PluginCreatorInterface>, PluginKeyHash> mPluginDict;
};

} // namespace logtail