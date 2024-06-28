#ifndef _MODULE_UTILS_H_
#define _MODULE_UTILS_H_

#include <string>
#include <vector>

namespace json {
    class Object;
}

#define LEN_4096 4096
#define LEN_1024 1024
// #define DELTA(t1, t2) ((t1)>(t2)?(t1)-(t2):0)
#define PERCENT(t1, t2) ((t2)!=0&&(t1)>0?(100.0*(t1))/(t2):0.0)
#define RATIO(t1, t2) ((t2)!=0&&(t1)>0?(1.0*(t1))/(t2):0.0)

namespace plugin {
    class PluginUtils {
    public:
        static std::string getLastNoneEmptyLineFromFile(const std::string &filePath);
        static int getKeys(const std::string &line, std::vector<std::string> &keyVector);
        static int getValues(const std::string &line, std::vector<long> &valueVector);
        static int GetSubDirs(const std::string &dirPath, std::vector<std::string> &subDirs);
        static uint64_t GetNumberFromFile(const std::string &file);
        static int64_t GetSignedNumberFromFile(const std::string &file);
        static void GetConfig(const json::Object &jsonValue, const std::string &key, std::vector<std::string> &);
    };
}
#endif
