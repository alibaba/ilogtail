#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif
#include "FileChannel.h"

#include <sstream>

#include "common/JsonValueEx.h"
#include "common/Logger.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/Chrono.h"

#include "common/Config.h"

using namespace std;
using namespace common;

namespace argus {
    FileChannel::FileChannel() :
            m_maxMsgNumber(1000),
            m_discardCount{0},
            m_clearIntv{std::chrono::hours{1}} {
    }

    FileChannel::~FileChannel() {
        endThread();
        join();
        //clear outputFileMap;
        {
            // std::unique_lock<std::mutex> _guard{m_outputFileMapLock};
            // for (auto it = m_outputFileMap.begin(); it != m_outputFileMap.end(); ++it) {
            //     std::shared_ptr<OutputFile> pFile = it->second;
            //     m_outputFileMap.erase(it++);
            //     pFile->count--;
            //     if (pFile->count == 0)
            //     {
            //         delete pFile;
            //     }
            // }
            m_outputFileMap.Clear();
            // m_outputFileMapLock.unlock();
        }
        //clear
        {
            // std::unique_lock<std::mutex> _guard{m_outputMsgListLock};
            m_outputMsgList.Clear();
            // while (m_outputMsgList.size() > 0) {
            //     OutputMsg *pMsg = m_outputMsgList.front();
            //     m_outputMsgList.pop_front();
            //     delete pMsg;
            // }
            // m_outputMsgListLock.unlock();
        }
    }

    void FileChannel::doRun() {
        const auto interval = std::chrono::microseconds{SingletonConfig::Instance()->getScheduleIntv()};
        const size_t totalCount = m_clearIntv / interval;
        LogInfo("FileChannel::doRun(interval: {} us, m_clearInterval: {} us, totalCount: {}",
                convert(interval.count(), true), convert(m_clearIntv.count(), true), totalCount);
        size_t clearCount = 0;
        while (!isThreadEnd()) {
            clearCount++;
            std::shared_ptr<OutputMsg> pMsg = popMsg();
            if (pMsg) {
                CpuProfile("FileChannel::doRun");

                Sync(m_outputFileMap) {
                    auto targetOutputFileIt = m_outputFileMap.find(pMsg->conf);
                    if (targetOutputFileIt == m_outputFileMap.end()) {
                        LogError("FileChannel logic error");
                    } else {
                        std::shared_ptr<OutputFile> pFile = targetOutputFileIt->second;
                        string content;
                        getMsgContent(*pMsg, content);
                        string error;
                        pFile->pFileWrite->doRecord(content.c_str(), content.size(), error);
                    }
                }}}
                // std::unique_lock<std::mutex> _outputFileGuard{m_outputFileMapLock};
                // auto targetOutputFileIt = m_outputFileMap.find(pMsg->conf);
                // if (targetOutputFileIt == m_outputFileMap.end()) {
                //     LogError("FileChannel logic error");
                // } else {
                //     std::shared_ptr<OutputFile> pFile = targetOutputFileIt->second;
                //     string content;
                //     getMsgContent(*pMsg, content);
                //     string error;
                //     pFile->pFileWrite->doRecord(content.c_str(), content.size(), error);
                //     // // 需要上报状态，并且写文件出现了错误
                //     // if (error.size() != 0)
                //     // {
                //     //    sendStatus(pMsg->reportStatus,pMsg->name,pMsg->time,WRITE_FILE_ERROR,error);
                //     // }
                // }
                // m_outputFileMapLock.unlock();
            } else {
                sleepFor(interval);
            }
            if (clearCount >= totalCount) {
                clearCount = 0;
                clearMap(m_clearIntv);
            }
        }
    }

    bool FileChannel::updateErrorConfMap(const std::string &conf, bool force) {
        bool updated = false;
        // std::unique_lock<std::mutex> _lock{m_errorConfMapLock};
        if (force || m_errorConfMap.Contains(conf)) {
            m_errorConfMap.Emplace(conf, std::chrono::system_clock::now());
            updated = true;
        }
        return updated;
    }

    void FileChannel::appendMessage(const std::shared_ptr<OutputMsg> &msg) {
        Sync(m_outputMsgList) {
            if (m_outputMsgList.size() >= m_maxMsgNumber) {
                m_outputMsgList.pop_front();
                m_discardCount++;
                LogError("discard one msg,discardCount={},maxMaxNumber={}", m_discardCount, m_maxMsgNumber);
            }
            m_outputMsgList.push_back(msg);
        }}}
    }

    std::shared_ptr<OutputMsg> FileChannel::popMsg() {
        std::shared_ptr<OutputMsg> msg{};

        m_outputMsgList.PopFront(&msg);

        return msg;
    }

    int FileChannel::addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                                 int exitCode, const std::string &result, const std::string &conf,
                                 bool reportStatus, const std::string &mid) {
        //判断conf是否已经是错误的，则直接返回
        if (updateErrorConfMap(conf, false)) {
            return -1;
        }

        std::shared_ptr<OutputFile> pFile{};
        if (!m_outputFileMap.Find(conf, pFile)) {
            pFile = getOutputFile(conf);
            if (pFile == nullptr) {
                //加入到errorMap
                updateErrorConfMap(conf, true);
                return -2;
            }
            m_outputFileMap.Emplace(conf, pFile);
        }
        pFile->lastTime = std::chrono::system_clock::now();

        auto pMsg = std::make_shared<OutputMsg>();
        pMsg->name = name;
        pMsg->exitCode = exitCode;
        pMsg->result = result;
        pMsg->conf = conf;
        pMsg->reportStatus = reportStatus;
        pMsg->time = timestamp; // 秒级utc时间戳
        appendMessage(pMsg);

        return 0;
    }

    int FileChannel::parseConf(const string &fileContent, FileConf &fileConf) {
        std::string error;
        json::Object value = json::parseObject(fileContent, error);
        if (value.isNull()) {
            LogError("conf is not a json object: {}, json: {}", error, fileContent);
            return -1;
        }
        // Json::Value pathValue = value.get(keyPath, Json::Value::null);
        // if (pathValue.isNull() || !pathValue.isString()) {
        //     return -1;
        // }
        // fileConf.path = pathValue.asString();
        fileConf.path = value.getString("path");
        // Json::Value sizeValue = value.get(keySize, Json::Value::null);
        // if (sizeValue.isNull() || !sizeValue.isInt()) {
        //     return -1;
        // }
        // fileConf.size = sizeValue.asInt();
        fileConf.size = value.getNumber<int>("size", -1);
        // Json::Value countValue = value.get(keyCount, Json::Value::null);
        // if (countValue.isNull() || !countValue.isInt()) {
        //     return -1;
        // }
        // fileConf.count = countValue.asInt();
        fileConf.count = value.getNumber<int>("count", -1);
        return fileConf.path.empty() || fileConf.size < 0 || fileConf.count < 0? -1: 0;
    }

    /*
        完全相同返回0，冲突返回1，不相关返回2
    */
    int FileChannel::confCompare(FileConf &fileConf1, FileConf &fileConf2) {
        if (fileConf1.path != fileConf2.path) {
            return 2;
        }
        if (fileConf1.size == fileConf2.size && fileConf1.count == fileConf2.count) {
            return 0;
        } else {
            return 1;
        }
    }

    //从现存的map中找是否有conf等同的配置
    std::shared_ptr<OutputFile> FileChannel::getOutputFile(const string &conf) {
        FileConf fileConf;
        // std::shared_ptr<OutputFile> pFile{};
        if (parseConf(conf, fileConf) != 0) {
            LogError("parse conf({}) error", conf);
            return nullptr;
        }

        bool found = false;
        auto targetFile = SyncCall(m_outputFileMap, {
            if (!m_outputFileMap.empty()) {
                found = true;
                for (const auto &it: m_outputFileMap) {
                    auto pFile = it.second;
                    int ret = confCompare(fileConf, pFile->fileConf);
                    if (ret == 0) {
                        return pFile;
                    } else if (ret == 1) {
                        LogError("conf (path={},size={},count={}) conflict with other conf(path={},size={},cont={})",
                                 fileConf.path, fileConf.size, fileConf.count,
                                 pFile->fileConf.path, pFile->fileConf.size, pFile->fileConf.count);
                        return std::shared_ptr<OutputFile>{};
                    }
                }
            }
            found = false;
            return std::shared_ptr<OutputFile>{};
        });
        if (found) {
            return targetFile;
        }

        //需要新创建
        auto pFileWrite = std::make_shared<FileWrite>(fileConf.size, fileConf.count);
        if (!pFileWrite->setup(fileConf.path)) {
            return nullptr;
        }
        auto pFile = std::make_shared<OutputFile>();
        pFile->fileConf = fileConf;
        pFile->pFileWrite = pFileWrite;
        // pFile->count = 1;
        m_outputFileMap.Emplace(conf, pFile);
        return pFile;
    }

    void FileChannel::getMsgContent(const OutputMsg &msg, string &content) {
        std::ostringstream oss;
        oss << ToSeconds(msg.time) << "|" << msg.name << "|" << msg.exitCode << "|" << msg.result;
        content = oss.str();
        std::replace(content.begin(), content.end(), '\n', ' ');
        content += "\n";
    }

    void FileChannel::clearMap(std::chrono::microseconds time) {
        const auto minTime = std::chrono::system_clock::now() - time;

        Sync(m_errorConfMap) {
            for (auto it = m_errorConfMap.begin(); it != m_errorConfMap.end();) {
                if (it->second < minTime) {
                    m_errorConfMap.erase(it++);
                } else {
                    it++;
                }
            }
        }}}

        Sync(m_outputFileMap) {
            for (auto it = m_outputFileMap.begin(); it != m_outputFileMap.end();) {
                if (it->second->lastTime < minTime) {
                    m_outputFileMap.erase(it++);
                } else {
                    it++;
                }
            }
        }}}
    }
}