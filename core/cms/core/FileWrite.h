#ifndef ARGUS_COMMON_FILE_WRITE_H
#define ARGUS_COMMON_FILE_WRITE_H

#include <string>
#include <vector>

struct apr_pool_t;
struct apr_file_t;

namespace argus {
    // struct RecordFileInfo {
    //     int64_t ctime;
    //     int64_t mtime;
    //     std::string name;
    // };

#include "common/test_support"
class FileWrite {
public:
    FileWrite(size_t size, size_t count);
    virtual ~FileWrite();
    bool setup(const std::string &file);
    int doRecord(const char *content, size_t writeLen, std::string &error);

private:
    int rotateLog();
    bool shouldRotate() const;
    // static bool finfoCompare(const RecordFileInfo &first, const RecordFileInfo &second);
    // //获取该日志的所有文件、并按照修改时间降序排列
    // size_t getAllLogFiles(std::vector<RecordFileInfo> &vecLogFiles,
    //                       const std::string &dirPath, const std::string &filename);

private:
    apr_pool_t *m_p = nullptr;
    apr_file_t *m_logFileDescriptor = nullptr;
    std::string m_fileName; //日志名称带完整路径
    size_t m_size = 0;      //单个日志文件的大小
    size_t m_count = 0;  //该日志的文件数量
};
#include "common/test_support"

}
#endif