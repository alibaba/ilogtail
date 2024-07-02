#ifndef ARGUS_COMMON_PROPERTIES_FILE_H
#define ARGUS_COMMON_PROPERTIES_FILE_H

#include <string>
#include <map>
#include "ConfigBase.h"
#include "common/FilePathUtils.h"

namespace common {
    class PropertiesFile : public ConfigBase {
    public:
        explicit PropertiesFile(const std::string &filename);
        ~PropertiesFile() = default;
        void AdjustPropertiesByFile(const fs::path &filename);
        int AdjustPropertiesByJsonFile(const fs::path &jsonFile);

        using ConfigBase::GetValue;
        std::string GetValue(const std::string &key, const std::string &defaultValue) const override;
#ifdef ENABLE_COVERAGE
        void Set(const std::string &key, const std::string &v) {
            mValueMap[key] = v;
        }
#endif
    private:
        std::map<std::string, std::string> mValueMap;
    };
}

#endif
