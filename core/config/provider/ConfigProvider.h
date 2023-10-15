#pragma once

#include <string>
#include <filesystem>

namespace logtail {
class ConfigProvider {
public:
    ConfigProvider(const ConfigProvider&) = delete;
    ConfigProvider& operator=(const ConfigProvider&) = delete;

    virtual void Init(const std::string& dir);

protected:
    ConfigProvider() = default;
    virtual ~ConfigProvider() = default;

    std::filesystem::path mSourceDir;
};
} // namespace logtail
