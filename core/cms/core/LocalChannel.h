#ifndef _LOCAL_CHANNEL_H_
#define _LOCAL_CHANNEL_H_

#include <list>
#include <vector>

#include "OutputChannel.h"
#include "common/InstanceLock.h"

namespace argus {
    struct LocalFileMsg {
        std::string msg;
        std::string moduleName;
        std::chrono::system_clock::time_point timeStamp;
    };

#include "common/test_support"
class LocalChannel : public OutputChannel {
public:
    LocalChannel();
    ~LocalChannel() override;
    int addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                    int exitCode, const std::string &result, const std::string &conf,
                    bool reportStatus, const std::string &mid) override;
private:
    void doRun() override;

    void dealMsg(const LocalFileMsg &msg);
    static std::string rotateFile(const std::string &file, std::chrono::system_clock::time_point timestamp);
    // static std::string getDateAsYYYYMMDD(const std::chrono::system_clock::time_point &timestamp); // 秒级时间戳
    static bool shouldRotate(const std::string &file,
                             const std::chrono::system_clock::time_point &timestamp);
    // static int getAllLogFiles(std::vector<FileInfo> &vecLogFiles, apr_pool_t *memPool,
    //                           std::string &dirPath, std::string &filename);
    virtual int outPut2File(const std::string &fileContent,
                    const std::chrono::system_clock::time_point &timeStamp,
                    const std::string &fileName);
    int removeOldFiles(std::string file, const std::chrono::system_clock::time_point &timestamp);
private:
    size_t m_finishCount = 0;
    size_t m_discardCount = 0;
    size_t m_maxCount = 200;
    size_t m_errorCount = 0;
    std::chrono::system_clock::time_point m_deleteTimestamp;
    std::list<LocalFileMsg> msgList;
    size_t m_logSaveDay = 7;
    size_t m_maxLogSize = 512 * 1000 * 1000;
    common::InstanceLock m_listLock;
    size_t mRecordSize = 51200;
};
#include "common/test_support"

}
#endif