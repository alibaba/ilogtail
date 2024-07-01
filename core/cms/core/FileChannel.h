#ifndef ARGUSAGENT_CORE_FILE_CHANNLE_H
#define ARGUSAGENT_CORE_FILE_CHANNLE_H

#include <map>
#include <string>
#include <list>
#include <mutex>
#include <chrono>

#include "OutputChannel.h"
#include "FileWrite.h"
#include "common/SafeMap.h"
#include "common/SafeVector.h"

namespace argus {
    struct OutputMsg {
        std::string name;
        int exitCode;
        std::string result;
        std::string conf;
        bool reportStatus;
        std::chrono::system_clock::time_point time;  // 秒级utc时间戳
    };
    struct FileConf {
        std::string path;
        int size;
        int count;
    };
    struct OutputFile {
        std::chrono::system_clock::time_point lastTime;
        FileConf fileConf;
        std::shared_ptr<FileWrite> pFileWrite;
        // int count = 0;
        int64_t lastSecondsTimestamp() const {
            using namespace std::chrono;
            return duration_cast<seconds>(lastTime.time_since_epoch()).count();
        }
    };

#include "common/test_support"
    class FileChannel : public OutputChannel {
    public:
        FileChannel();
        ~FileChannel() override;
        int addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                        int exitCode, const std::string &result, const std::string &conf,
                        bool reportStatus, const std::string &mid) override;

    private:
        void doRun() override;
        static int parseConf(const std::string &fileContent, FileConf &fileConf);
        //完全相同返回0，冲突返回1，不相关返回2
        static int confCompare(FileConf &fileConf1, FileConf &fileConf2);
        std::shared_ptr<OutputFile> getOutputFile(const std::string &conf);
        static void getMsgContent(const OutputMsg &pMsg, std::string &content);
        void clearMap(std::chrono::microseconds time);

        bool updateErrorConfMap(const std::string &conf, bool force);
        void appendMessage(const std::shared_ptr<OutputMsg>& msg);
        std::shared_ptr<OutputMsg> popMsg();
    private:
        // std::mutex m_outputFileMapLock;
        SafeMap<std::string, std::shared_ptr<OutputFile>> m_outputFileMap;

        // std::mutex m_outputMsgListLock;
        SafeList<std::shared_ptr<OutputMsg>> m_outputMsgList;

        // std::mutex m_errorConfMapLock;
        SafeMap<std::string, std::chrono::system_clock::time_point> m_errorConfMap;

        const size_t m_maxMsgNumber;
        int m_discardCount;
        std::chrono::microseconds m_clearIntv;
    };
#include "common/test_support"

}
#endif
