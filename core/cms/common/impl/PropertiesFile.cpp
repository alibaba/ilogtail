#include "common/PropertiesFile.h"
#include "common/FileUtils.h"

#include "common/JsonValueEx.h"
#include "common/Logger.h"

using namespace std;

namespace common {
    PropertiesFile::PropertiesFile(const std::string &filename) {
        if (!filename.empty()) {
            AdjustPropertiesByFile(fs::path{filename});
        }
    }

    std::string PropertiesFile::GetValue(const std::string &key, const std::string &defaultValue) const {
        auto it = mValueMap.find(key);
        return it != mValueMap.end() ? it->second : defaultValue;
    }

    void PropertiesFile::AdjustPropertiesByFile(const fs::path &filename) {
        std::vector<std::string> lines;
        FileUtils::ReadFileLines(filename.string(), lines);
        for (std::string line : lines) {
            line = TrimLeftSpace(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            size_t index = line.find('=');
            if (index != string::npos) {
                string key = TrimSpace(line.substr(0, index));
                string value = TrimSpace(line.substr(index + 1));
                if (!key.empty() && !value.empty()) {
                    mValueMap[key] = value;
                }
            }
        }
    }

    int PropertiesFile::AdjustPropertiesByJsonFile(const fs::path &jsonFile) {
        enum {
            FILE_NOT_EXIST = -1,
            FILE_CONTENT_EMPTY = -2,
            FILE_CONTENT_INVALID = -3,
        };

        bool ok = fs::exists(jsonFile);
        if (ok) {
            string content;
            FileUtils::ReadFileContent(jsonFile.string(), content);
            if (content.empty()) {
                LogError("agent config file <{}> is empty, skip!", jsonFile.string());
                return FILE_CONTENT_EMPTY; // 配置为空，或不存在，则忽略
            }

            std::string error;
            json::Object root = json::parseObject(content, error);
            if (root.isNull()) {
                LogError("agent config file <{}> invalid: {}", jsonFile.string(), error);
                return FILE_CONTENT_INVALID;
            }
            for (const auto &it: *root.native()) {
                if (!it.key().empty()) {
                    std::string value = json::valueToString(&it.value());
                    if (!value.empty()) {
                        mValueMap[it.key()] = value;
                    }
                }
            }
        }
        return ok? 0: FILE_NOT_EXIST;
	}
}
