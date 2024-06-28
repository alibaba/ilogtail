#include "LocalChannel.h"

#include <chrono>

#include "common/Config.h"
#include "common/Logger.h"
#include "common/Common.h"
#include "common/ScopeGuard.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/SyncQueue.h"
#include "common/Chrono.h"

using namespace std;
using namespace std::chrono;
using namespace common;

namespace argus
{
LocalChannel::LocalChannel()
{
    m_finishCount =0;
    m_discardCount=0;
    m_errorCount=0;
    m_maxCount=200;
    m_logSaveDay=7;
    SingletonConfig::Instance()->GetValue("agent.local.record.max.total.size", m_maxLogSize, 512*1000*1000);
    SingletonConfig::Instance()->GetValue("agent.local.record.max.size", mRecordSize, 51200);
    m_deleteTimestamp = std::chrono::Now<BySeconds>() + std::chrono::seconds{randValue(3600, 20 * 3600)};
    LogInfo("m_deleteTimestamp={:L%F %T %z}",m_deleteTimestamp);
	// apr_pool_create(&m_p, nullptr);
}

int LocalChannel::addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                              int exitCode, const std::string &result, const std::string &conf,
                              bool reportStatus, const std::string &mid){
    //对result进行判断
    if (result.size() > mRecordSize) {
        LogInfo("the result(size={}) is too large,will skip to record to local!", result.size());
        return -1;
    }
    LocalFileMsg localMsg;
    localMsg.msg=result;
    localMsg.moduleName=name;
    localMsg.timeStamp = timestamp;
    m_listLock.lock();
    if(msgList.size()>=m_maxCount){
        msgList.pop_front();
        m_discardCount++;
    }
    msgList.push_back(localMsg);
    m_listLock.unlock();
    return 0;
}
void LocalChannel::doRun(){
    const auto interval = microseconds{SingletonConfig::Instance()->getScheduleIntv()};
    while (!isThreadEnd()) {
        LocalFileMsg msg;
        if (PopQueue(m_listLock, msgList, msg)) {
            CpuProfile("HttpMetricCollect::doRun");

            m_finishCount++;
            dealMsg(msg);
        } else {
            sleepFor(interval);
        }
        //
        // m_listLock.lock();
        // if (!msgList.empty()) {
        //     LocalFileMsg msg = msgList.front();
        //     msgList.pop_front();
        //     m_listLock.unlock();
        //     m_finishCount++;
        //     dealMsg(msg);
        // } else{
        //     m_listLock.unlock();
        //     sleepFor(microseconds{intv});
        // }
    }
}
LocalChannel::~LocalChannel()
{
    endThread();
    join();
	// apr_pool_destroy(m_p);
}

void LocalChannel::dealMsg(const LocalFileMsg &msg) {
    //写入的方式为
    // string noLineMsg = msg.msg;
    string time = convert(ToSeconds(msg.timeStamp));
    string writeMsg = time + " " + msg.msg + "\n";
    auto filePath = SingletonConfig::Instance()->getModuleDataDir();
    // //创建目录
    // if (apr_dir_make_recursive(filePath.string().c_str(), 0x755, m_p) != APR_SUCCESS) {
    //     LogError("create dir({}) error", filePath.c_str());
    //     return;
    // }
    boost::system::error_code ec;
    if (!fs::exists(filePath) && !fs::create_directories(filePath, ec)) {
        LogError("create directories ({}) error: {}", filePath.string(), ec.message());
        return;
    }

    std::string fileName = (filePath / (msg.moduleName + ".data")).string();
    outPut2File(writeMsg, msg.timeStamp, fileName);
}

int LocalChannel::outPut2File(const string &fileContent,
                              const std::chrono::system_clock::time_point &timeStamp,
                              const string &fileName) {
    argus::LocalChannel::rotateFile(fileName, timeStamp);
    removeOldFiles(fileName, timeStamp);

    FILE *fp = fopen(fileName.c_str(), "a");
    if (!fp) {
        LogError("output_file: can't open or create data file = {}  err={}", fileName, errno);
        return -1;
    }
    defer(fclose(fp));

    if (fputs(fileContent.c_str(), fp) < 0) {
        LogError("write line error");
    }

    return 0;
}

std::string LocalChannel::rotateFile(const string &file, std::chrono::system_clock::time_point timestamp) {
    // //仅仅回滚即可，去除之前的0点删除逻辑
    // apr_pool_t *memPool = nullptr;
    // apr_allocator_t* allocator;
    // apr_allocator_create(&allocator);
    // if(allocator==nullptr)
    // {
    //     LogError("apr_allocator_create error");
    //     return;
    // }
    // apr_pool_create_ex(&memPool, nullptr, nullptr, allocator);
    // apr_allocator_owner_set(allocator, memPool);
    // ResourceGuard rg([memPool]() {if (memPool) {apr_pool_destroy(memPool);}});

    std::string oldLogFile;
	// if the filename exists, change it into the "filename.yyyymmdd" as old log file.
    if (argus::LocalChannel::shouldRotate(file, timestamp)) {
		// apr_time_exp_t expt;
        // //获取前一天的日期
		// apr_status_t rv = apr_time_exp_lt(&expt, (timestamp-24*60*60)*1000*1000);
		// if (rv != APR_SUCCESS)
		// {
		// 	return;
		// }
		//char oldLogFile[1024];
//#ifdef WIN32
//		_snprintf(oldLogFile, sizeof(oldLogFile), "%s.%04d%02d%02d", file.c_str(), expt.tm_year + 1900, expt.tm_mon + 1, expt.tm_mday);
//#else
//		snprintf(oldLogFile, sizeof(oldLogFile), "%s.%04d%02d%02d", file.c_str(), expt.tm_year + 1900, expt.tm_mon + 1, expt.tm_mday);
//#endif
		// apr_file_rename(file.c_str(), oldLogFile.c_str(), memPool);
		oldLogFile = fmt::format("{}.{:L%Y%m%d}", file, timestamp - std::chrono::hours{24});
        fs::rename(file, oldLogFile);
    }
    return oldLogFile;
}
int LocalChannel::removeOldFiles(string file, const std::chrono::system_clock::time_point &timestamp)
{
    //先找个随机的时间，不能0点
    if(timestamp < m_deleteTimestamp){
       return -1;
    }
    m_deleteTimestamp = timestamp + std::chrono::hours{24}; // 24*3600;
    LogInfo("next m_deleteTimestamp={:L%F %T %z}",m_deleteTimestamp);
    // apr_pool_t *memPool = nullptr;
    // apr_allocator_t* allocator;
    // apr_allocator_create(&allocator);
    // if(allocator==nullptr)
    // {
    //     LogError("apr_allocator_create error");
    //     return;
    // }
    // apr_pool_create_ex(&memPool, nullptr, nullptr, allocator);
    // apr_allocator_owner_set(allocator, memPool);
    // ResourceGuard rg([memPool]() {if (memPool) {apr_pool_destroy(memPool);}});

    // string::size_type lastSlashPosition = file.rfind('/');
    // string::size_type lastSlashPositionExt = lastSlashPosition + 1;
    // if (string::npos == lastSlashPosition)
    // {
    //     lastSlashPosition = file.rfind('\\');
    //     lastSlashPositionExt = lastSlashPosition + 1;
    //     if (string::npos == lastSlashPosition)
    //     {
    //         LogError("file[{}] must in folder.", file);
    //         return;
    //     }
    // }
    fs::path filePath{file};
    fs::path dirPath = filePath.parent_path();
    if (dirPath.empty()) {
        LogError("file <{}>must be in folder", file);
        return -2;
    }
    size_t totalSize = 0;
    int rmCount = 0;
    EnumerateLoggerFiles(dirPath, std::string{}, [&](const LoggerFileInfo &f) {
        LogInfo("fileName={}, mtime={:L}", f.file.filename().string(), f.mtime);
        if (f.mtime + 24_h * (m_logSaveDay + 1) < timestamp) {
            // char *fullFilePath = nullptr;
            // apr_filepath_merge(&fullFilePath, dirPath.c_str(),finfo.name,APR_FILEPATH_NATIVE, memPool);
            // apr_file_remove(fullFilePath, memPool);
            boost::system::error_code ec;
            fs::remove(f.file, ec);
            rmCount++;
            LogInfo("remove old file <{}>: {}", f.file.string(), (ec.failed() ? ec.message() : std::string{"success"}));
        } else {
            boost::system::error_code ec;
            auto size = fs::file_size(f.file, ec);
            totalSize += (ec.failed() ? 0 : size);
        }
    });

    // string dirPath = file.substr(0, lastSlashPosition);
//     //删除这个目录中超过m_logSaveDay中的日志文件
//     apr_dir_t *dir = nullptr;
// 	if(apr_dir_open(&dir, dirPath.c_str(), memPool)!=APR_SUCCESS)
//     {
//         return;
//     }
// 	apr_finfo_t finfo;
// 	int finfoFlags = APR_FINFO_TYPE | APR_FINFO_NAME | APR_FINFO_CTIME|APR_FINFO_MTIME;
// 	apr_status_t apr_ret = 0;
//     size_t totalSize=0;
// 	while (APR_ENOENT != apr_ret)
// 	{
// 		apr_ret = apr_dir_read(&finfo, finfoFlags, dir);
// #ifdef WIN32
// 		if (720018 == apr_ret)
// 		{
// 			break;
// 		}
// #else
// 		if (APR_ENOENT == apr_ret)
// 		{
// 			break;
// 		}
// #endif
//         auto mtime = FromMicros(finfo.mtime);
//         LogInfo("fileName={},mtine={:L}",finfo.name, mtime);
//         if (mtime + std::chrono::hours{24} * (m_logSaveDay + 1) < timestamp) {
//             char *fullFilePath = nullptr;
//             apr_filepath_merge(&fullFilePath, dirPath.c_str(),finfo.name,APR_FILEPATH_NATIVE, memPool);
//             apr_file_remove(fullFilePath, memPool);
//             LogInfo("remove oldfile={}",fullFilePath);
//         }else{
//             totalSize +=finfo.size;
//         }
// 	}
// 	apr_dir_close(dir);
    size_t saveDay = m_logSaveDay;
    if (totalSize > 0) {
        saveDay = m_maxLogSize / totalSize;
        saveDay = saveDay > 7 ? 7 : saveDay;
        saveDay = saveDay == 0 ? 1 : saveDay;
        m_logSaveDay = saveDay;
    }
    LogDebug("in removeOldFiles,sayDay={},totalSize={}", saveDay, totalSize);

    return rmCount;
}
// string LocalChannel::getDateAsYYYYMMDD(const std::chrono::system_clock::time_point &timestamp)
// {
// //      apr_time_exp_t expt;
// //     //获取前一天的日期
// //     apr_status_t rv = apr_time_exp_lt(&expt, apr_time_from_sec(timeStamp));
// //     if (rv != APR_SUCCESS)
// //     {
// //         return "";
// //     }
// //     char buf[1024];
// // #ifdef WIN32
// //     int result = _snprintf(buf, sizeof(buf), "%04d%02d%02d",expt.tm_year + 1900, expt.tm_mon + 1, expt.tm_mday);
// //     if (result == sizeof(buf) || result < 0)
// //     {
// //         buf[sizeof(buf) - 1] = 0;
// //     }
// // #else
// //     snprintf(buf, sizeof(buf), "%04d%02d%02d", expt.tm_year + 1900, expt.tm_mon + 1, expt.tm_mday);
// // #endif
// //
// //     return string(buf);
//     return fmt::format("{:L%F}", timestamp);
// }

bool LocalChannel::shouldRotate(const string &file,
                                const std::chrono::system_clock::time_point &timestamp) {
    auto align = [](int64_t seconds) {
        return seconds - seconds % (24 * 60 * 60);
    };

    boost::system::error_code ec;
    auto mtime = fs::last_write_time(file, ec);
    if (!ec.failed() && align(mtime) != align(ToSeconds(timestamp) + 1)) {
        // 测试时不sleep
#if !defined(ENABLE_COVERAGE)
        std::this_thread::sleep_for(std::chrono::seconds{1});
#endif
        return true;
    }
    return false;
    // apr_finfo_t fileInfo;
    //
    // auto align = [](int64_t micros) {
    //     auto seconds = micros / (1024 * 1024);
    //     return seconds - seconds % (24 * 3600);
    // };
    //
    // apr_status_t rv = apr_stat(&fileInfo,file.c_str(), APR_FINFO_MTIME, memPool);
    // if (0 == rv && align(fileInfo.mtime) != align(ToMicros(timestamp) + 1)) {
    //     std::this_thread::sleep_for(std::chrono::seconds{1});
    //     return true;
    // }
	//
    // return false;

}

// int LocalChannel::getAllLogFiles(std::vector<FileInfo> &vecLogFiles, apr_pool_t *memPool,
//                                  string &dirPath, string &filename) {
// 	if(nullptr == memPool)
// 	{
// 		return 0;
// 	}
//
//
// 	apr_dir_t *dir = nullptr;
// 	if(apr_dir_open(&dir, dirPath.c_str(), memPool)!=APR_SUCCESS)
//     {
//         return 0;
//     }
//
// 	apr_finfo_t finfo;
// 	int finfoFlags = APR_FINFO_TYPE | APR_FINFO_NAME | APR_FINFO_CTIME|APR_FINFO_MTIME;
//
// 	apr_status_t apr_ret = 0;
// 	while (APR_ENOENT != apr_ret)
// 	{
// 		apr_ret = apr_dir_read(&finfo, finfoFlags, dir);
//
// #ifdef WIN32
// 		if (720018 == apr_ret)
// 		{
// 			break;
// 		}
// #else
// 		if (APR_ENOENT == apr_ret)
// 		{
// 			break;
// 		}
// #endif
//
// 		FileInfo fileInfo;
//
// 		if (nullptr != strstr(finfo.name, filename.c_str()))
// 		{
// 			fileInfo.ctime = finfo.ctime;
// 			fileInfo.mtime = finfo.mtime;
// 			fileInfo.name = string(finfo.name);
// 			vecLogFiles.push_back(fileInfo);
//
// 		}
// 	}
//
// 	if(vecLogFiles.size()>1)
// 	{
// 		sort(vecLogFiles.begin(), vecLogFiles.end(), [](const FileInfo &first, const FileInfo &second) {
//             //return first.ctime >= second.ctime;
//             // can't use ctime for windows，because the ctime of the files is same in some dir.just can use mtime .
//             return first.mtime > second.mtime;
//         });
// 	}
//
// 	apr_dir_close(dir);
//
// 	return static_cast<int>(vecLogFiles.size());
//
// }
}
