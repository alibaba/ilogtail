#include <vector>
#include <sstream>

#include "common/Logger.h"
#include "common/NetWorker.h"
#include "common/Config.h"
#include "common/FilePathUtils.h"
#include "common/Chrono.h"
#include "common/UnitTestEnv.h"

#include "FileWrite.h"

#include <apr-1/apr_file_io.h>

#ifdef max
#   undef max
#endif

using namespace common;
using namespace std;

namespace argus
{
FileWrite::FileWrite(size_t size, size_t count) {
	apr_pool_create(&m_p, nullptr);
    m_logFileDescriptor = nullptr;;
    this->m_size = std::max<size_t>(size, 1000 * 1000);
    this->m_count = std::max<size_t>(count, 2);
}

FileWrite::~FileWrite() 
{
	if(m_logFileDescriptor!=nullptr){
		apr_file_close(m_logFileDescriptor);
		m_logFileDescriptor=nullptr;
	}
	apr_pool_destroy(m_p);
}

bool FileWrite::setup(const std::string &file) 
{
    fs::path path{file};
    fs::path dir = path.parent_path();
    fs::path filename = path.filename(); // file.substr(lastSlashPosition + 1);
    if (dir.empty() || filename.empty()) {
        return false;
    }
    if (dir.is_relative()) {
        fs::path baseDir = SingletonConfig::Instance()->getBaseDir();
        dir = baseDir / dir;
    }

    std::string finalFile = (dir / filename).string();
    boost::system::error_code ec;
    fs::create_directories(dir, ec);
    if (ec.failed()) {
        LogError("fs::create_directories({}) failed: {}", dir.string(), ec.message());
        return false;
    }
    // 0x0755
    fs::permissions(dir, fs::owner_all | fs::group_read | fs::group_exe | fs::others_read | fs::others_exe, ec);
    if (ec.failed()) {
        LogError(R"(fs::set_permissions("{}", "rwxrw-rw-") failed: {})", dir.string(), ec.message());
        return false;
    }
	// apr_status_t ret = apr_dir_make_recursive(dir.string().c_str(), 0x755, m_p);
	// if (APR_SUCCESS != ret)
	// {
	// 	LogError("can not open dir [{}], errorMessage is [{}]", finalFile, strerror(errno));
	// 	return false;
	// }
    int32_t flags = APR_CREATE | APR_APPEND | APR_WRITE;
    const apr_fileperms_t perms = APR_FPROT_UREAD | APR_FPROT_UWRITE | APR_FPROT_GREAD | APR_FPROT_WREAD;
	apr_status_t ret = apr_file_open(&m_logFileDescriptor, finalFile.c_str(), flags, perms, m_p);
	if (ret != APR_SUCCESS) 
	{
		LogError("open file [{}] failed, errorMessage is [{}]", finalFile, strerror(errno));
		return false;
	}

	m_fileName = finalFile;
    LogInfo("FileWriter::setup({})", m_fileName);
	return true;
}


bool FileWrite::shouldRotate() const
{
    apr_finfo_t fileInfo;
    apr_status_t rv = apr_file_info_get(&fileInfo, APR_FINFO_SIZE, m_logFileDescriptor);
    return 0 == rv && size_t(fileInfo.size) > m_size;
}

// size_t FileWrite::getAllLogFiles(std::vector<RecordFileInfo>& vecLogFiles, const string &dirPath, const string &filename)
// {
//     apr_dir_t *dir = nullptr;
//     if (apr_dir_open(&dir, dirPath.c_str(), m_p) != APR_SUCCESS) {
//         return 0;
//     }
//     defer(apr_dir_close(dir));
//
//     apr_finfo_t finfo;
//     const int finfoFlags = APR_FINFO_TYPE | APR_FINFO_NAME | APR_FINFO_CTIME | APR_FINFO_MTIME;
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
// 		RecordFileInfo recordFileInfo;
//
// 		if (nullptr != strstr(finfo.name, filename.c_str()))
// 		{
// 			recordFileInfo.ctime = finfo.ctime;
// 			recordFileInfo.mtime = finfo.mtime;
// 			recordFileInfo.name = string(finfo.name);
// 			vecLogFiles.push_back(recordFileInfo);
// 			//printf("mtime=%llu,name=%s\n",finfo.mtime,finfo.name);
// 		}
// 	}
// 	if(vecLogFiles.size()>1)
// 	{
// 		sort(vecLogFiles.begin(), vecLogFiles.end(), &(FileWrite::finfoCompare));
// 	}
//
// 	return vecLogFiles.size();
//
// }

int FileWrite::rotateLog() 
{
	if (this->shouldRotate()) 
	{
        int64_t timeStamp= NowSeconds();
        std::ostringstream oss;
        oss << m_fileName << "." << timeStamp;
        std::string oldLogFile = oss.str();

        apr_file_close(this->m_logFileDescriptor);
        this->m_logFileDescriptor = nullptr;
		// apr_file_rename(m_fileName.c_str(), oldLogFile.c_str(),m_p);
		fs::rename(m_fileName, oldLogFile);
		//maybe remove oldLogFile
		//string::size_type lastSlashPosition = this->m_fileName.rfind("/");
		//string::size_type lastSlashPositionExt = lastSlashPosition + 1;
		//if (string::npos == lastSlashPosition)
		//{
		//	lastSlashPosition = this->m_fileName.rfind("\\");
		//	lastSlashPositionExt = lastSlashPosition + 1;
		//	if (string::npos == lastSlashPosition)
		//	{
		//		LogError("file({}) should in folder", m_fileName.c_str());
		//		return -1;
		//	}
		//}
		fs::path filePath{ this->m_fileName };
		string dirPath = filePath.parent_path().string();
		if (dirPath.empty()) {
			LogError("file({}) should in folder", m_fileName);
			return -1;
		}
		string filename = filePath.filename().string();

        // vector<RecordFileInfo> vecLogFiles;
        // size_t nLogCount = this->getAllLogFiles(vecLogFiles, dirPath, filename);
        std::vector<LoggerFileInfo> vecLogFiles = EnumerateLoggerFiles(dirPath, filename, true);
        for (size_t i = this->m_count; i < vecLogFiles.size(); i++) {
            // char *filepath = nullptr;
            // pool长期持有的情况下，apr_filepath_merge会有内存泄漏
            // apr_filepath_merge(&filepath, dirPath.c_str(), vecLogFiles[i].name.c_str(), APR_FILEPATH_NATIVE, m_p);
            // apr_file_remove(filepath, m_p);
            boost::system::error_code ec;
            fs::remove(vecLogFiles[i].file, ec);
            if (ec.failed()) {
                LogError("fs::remove({}), error: {}", vecLogFiles[i].file.string(), ec.message());
            }
        }

        int32_t flags = APR_CREATE | APR_APPEND | APR_WRITE;
        const apr_fileperms_t perms = APR_FPROT_UREAD | APR_FPROT_UWRITE | APR_FPROT_GREAD | APR_FPROT_WREAD;
		if (apr_file_open(&m_logFileDescriptor, m_fileName.c_str(), flags, perms, m_p) != APR_SUCCESS)
		{
            LogError("create file({}) error", m_fileName);
			return -1;
		}
	}
	return 0;
}

int FileWrite::doRecord(const char *content, size_t writeLen, std::string &error) {
    if (m_logFileDescriptor == nullptr) {
        LogError("the record file({}) is not opened", m_fileName);
        return -1;
    }

#define RETURN_ON_FAIL(f, ...)  do {                                         \
        apr_status_t rv = f(__VA_ARGS__);                                    \
        if (rv != APR_SUCCESS) {                                             \
            error = NetWorker::ErrorString(rv);                              \
            LogError("{}(\"{}\") error: ({}){}", #f, m_fileName, rv, error); \
            return -1;                                                       \
        }                                                                    \
    }while(false)

    RETURN_ON_FAIL(apr_file_write, m_logFileDescriptor, content, &writeLen);
    RETURN_ON_FAIL(apr_file_flush, m_logFileDescriptor);

#undef RETURN_ON_FAIL

    rotateLog();
    return static_cast<int>(writeLen);
}


// bool FileWrite::finfoCompare(const RecordFileInfo &first, const RecordFileInfo &second)
// {
//     //return first.ctime >= second.ctime;
//     // can't use ctime for windows，because the ctime of the files is same in some dir.just can use mtime .
//     return first.mtime > second.mtime;
// }
}



