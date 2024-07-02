#ifndef ARGUS_COMMON_REGEX_UTIL_H
#define ARGUS_COMMON_REGEX_UTIL_H

#include <string>
#include <vector>

namespace common {

    class RegexUtil {
    public:
        enum {
            MaxMatch = 10
        };
        static int match_first(const std::string &s, const std::string &pattern, std::string &out);
        // 最大不超过10个
        static int match(const std::string &s, const std::string &pattern, size_t count, std::vector<std::string> &);
        static bool match(const std::string &s, const std::string &pattern);
    };

}

#endif
