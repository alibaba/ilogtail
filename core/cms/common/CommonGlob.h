#ifndef WIN32

#include <vector>
#include <string>

namespace common {

#include "test_support"
class globUtil {
public:
    //对于posix glob()的封装，不支持**递归,禁止了./**或../**或~/**的匹配,不区分文件和目录。
    static void myglob1(const std::string &pattern, std::vector<std::string> &fileNames);
    //对于posix glob()的加强，支持**递归，区分文件和目录。
    static void myglob2(const std::string &pattern, std::vector<std::string> &fileNames);
    //检查文件名是否匹配一个glob表达式（支持*递归，不好用）
    //static bool wildMatch(const std::string &pattern, const std::string &filename);
private:
    static void adjustPattern(const std::string &pattern, std::vector<std::string> &patterns);
    static void globHandler(const std::string &pattern, std::vector<std::string> &fileNames);
};
#include "test_support"

}
#endif