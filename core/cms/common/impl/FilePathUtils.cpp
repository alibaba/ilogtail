#ifdef WIN32
#	pragma warning(disable: 4819)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#endif
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define  _CRT_SECURE_NO_WARNINGS  // warning C4996: 'fopen': This function or variable may be unsafe
#endif

#include "common/FilePathUtils.h"
#include "common/StringUtils.h"
#include "common/Logger.h"

#include "common/TimeFormat.h"

#ifdef WIN32
#include <Windows.h>
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <libproc.h>
#include <unistd.h>
#endif

#ifndef WIN32
std::string ReadLink(const std::string &path) {
    char buffer[PATH_MAX];
    ssize_t len = readlink(path.c_str(), buffer, sizeof(buffer));
    if (len < 0) {
        return {};
    }
    return {buffer, buffer + len};
}
#endif

fs::path GetExecPath() {
#if defined(WIN32)
    char szPath[MAX_PATH + 1] = { 0 };
    //DWORD len =
    GetModuleFileName(NULL, szPath, MAX_PATH);

    // std::cout << "ModuleFileName: " << szPath << std::endl;
    return szPath;
#elif defined(__linux__) || defined(__unix__)
    return ReadLink("/proc/self/exe");
#elif defined(__APPLE__) || defined(__FreeBSD__)
    std::vector<char> pathBuf;
    pathBuf.resize(PROC_PIDPATHINFO_MAXSIZE);
    int ret = proc_pidpath(static_cast<int>(getpid()), &pathBuf[0], PROC_PIDPATHINFO_MAXSIZE);
    if (ret <= 0) {
        pathBuf[0] = '\0';
    }
    return fs::path{&pathBuf[0]};
#endif
}

fs::path GetExecDir() {
    return GetExecPath().parent_path();
}

fs::path GetBaseDir() {
    // 实际安装路为：${INSTALL_DIR}/bin/argus_agent，或${SOURCE_ROOT}/test/unit_test
    fs::path baseDirectory{GetExecDir()};
    if (baseDirectory.filename() == "bin") {
        baseDirectory = baseDirectory.parent_path();
    }

    if (fs::exists(__FILE__)) {
        fs::path sourceRoot(__FILE__); // 源码根目录
        sourceRoot = sourceRoot.parent_path(); // 去掉文件名

        const std::string expectedLeaves[] = {"src", "sic", "src"};
        for (const auto &leaf: expectedLeaves) {
            if (sourceRoot.filename() == leaf) {
                sourceRoot = sourceRoot.parent_path();
            }
        }

        if (baseDirectory == sourceRoot) {
            baseDirectory = sourceRoot / "bin";
        }
    }

    return baseDirectory;
}

#if !defined(WIN32)
fs::path PATH[]{
        "/bin",
        "/sbin",

        "/usr/bin",
        "/usr/sbin",

        "/usr/local/bin",
        "/usr/local/sbin",
};
fs::path Which(const std::string &binFile, const std::initializer_list<fs::path> &extraPath) {
    for (const auto &root: PATH) {
        if(fs::exists(root / binFile)) {
            return root / binFile;
        }
    }
    for (const auto &root: extraPath) {
        if(fs::exists(root / binFile)) {
            return root / binFile;
        }
    }
    return fs::path{};
}
#endif


bool RemoveFile(const fs::path &file) {
    boost::system::error_code ec;
    bool removed = fs::remove(file, ec);
    bool ok = !ec.failed();
    if (!ok || !removed) {
        LogWarn("fs::remove({}) => {}, ec: {}", file.string(), removed, ec.message());
    }
    return ok;
}

namespace common {
    std::string FilePathUtils::GetDirPath(const std::string &filePath) {
        // size_t lastSlashPosition = filePath.rfind("/");
        // if (std::string::npos == lastSlashPosition)
        // {
        // 	lastSlashPosition = filePath.rfind("\\");
        // 	if (std::string::npos == lastSlashPosition)
        // 	{
        // 		return "";
        // 	}
        // }
        // return filePath.substr(0, lastSlashPosition);
        return fs::path{filePath}.parent_path().string();
    }

    std::string FilePathUtils::GetFileName(const std::string &filePath) {
        // size_t lastSlashPosition = filePath.rfind("/");
        // if (std::string::npos == lastSlashPosition)
        // {
        // 	lastSlashPosition = filePath.rfind("\\");
        // 	if (std::string::npos == lastSlashPosition)
        // 	{
        // 		return "";
        // 	}
        // }
        // return filePath.substr(lastSlashPosition+1);
        return fs::path(filePath).filename().string();
    }

    std::string FilePathUtils::GetFilePath(const std::string &dir, const std::string &fileName) {
        // #ifdef WIN32
        //     return dir+"\\"+fileName;
        // #else
        //     return dir+"/"+fileName;
        // #endif
        return (fs::path(dir) / fileName).string();
    }

    uintmax_t FilePathUtils::GetFileSize(const std::string &filePath) {
        boost::system::error_code ec;
        auto size = fs::file_size(filePath, ec);
        return ec.failed() ? 0 : size;
        // apr_pool_t *mp;
        // apr_pool_create(&mp, NULL);
        // int finfoFlags = APR_FINFO_NORM;
        // apr_finfo_t finfo;
        // finfo.ctime = 0;
        // finfo.mtime = 0;
        // finfo.size = 0;
        // if(apr_stat(&finfo,filePath.c_str(),finfoFlags,mp)!=APR_SUCCESS)
        // {
        // 	apr_pool_destroy(mp);
        // 	return finfo;
        // }
        // apr_pool_destroy(mp);
        // return finfo;
    }

    bool FilePathUtils::SetFileMTime(const boost::filesystem::path &file, const std::chrono::system_clock::time_point &tp) {
        using namespace std::chrono;
        boost::system::error_code ec;
        boost::filesystem::last_write_time(file, std::chrono::system_clock::to_time_t(tp), ec);
        if (ec.failed()) {
            LogError("SetFileMTime(<{}>, {:L}) => {}", file.string(), tp, ec.message());
        }
        return !ec.failed();
    }

    std::chrono::system_clock::time_point FilePathUtils::GetFileMTime(const fs::path &file) {
        std::chrono::system_clock::time_point mtime;

        boost::system::error_code ec;
        std::time_t lastWriteTime = fs::last_write_time(file, ec);
        if (!ec.failed()) {
            mtime = std::chrono::system_clock::from_time_t(lastWriteTime);
        } else {
            LogDebug("GetFileMTime({}) => {}", file.string(), ec.message());
        }
        return mtime;
    }

    std::string FilePathUtils::GetLastFile(const std::string &dirPath, const std::string &fileName,
                                           std::chrono::system_clock::time_point lastSaveTime) {
        // 	apr_pool_t *mp;
        //     apr_pool_create(&mp, NULL);
        // 	apr_dir_t *dir = NULL;
        // 	std::string filePath = GetFilePath(dirPath,fileName);
        // 	if(apr_dir_open(&dir, dirPath.c_str(),mp)!=APR_SUCCESS)
        //     {
        //         apr_pool_destroy(mp);
        // 		return filePath;
        // 	}
        // 	apr_finfo_t finfo;
        // 	apr_finfo_t currentFinfo;
        // 	currentFinfo.mtime =0;
        // 	int finfoFlags = APR_FINFO_TYPE | APR_FINFO_NAME | APR_FINFO_CTIME|APR_FINFO_MTIME;
        // 	apr_status_t apr_ret = 0;
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
        // 		//LogDebug("finfo.name={},fileName={}",finfo.name,fileName.c_str());
        //         if(finfo.filetype ==APR_REG &&StringUtils::StartWith(std::string(finfo.name),fileName))
        //         {
        // 			//找到修改时间大于lastSaveTime的最新文件,不能是文件本身
        //             if(finfo.mtime>lastSaveTime &&finfo.mtime>currentFinfo.mtime &&std::string(finfo.name)!=fileName)
        // 			{
        // 				currentFinfo = finfo;
        // 				filePath = GetFilePath(dirPath,std::string(finfo.name));
        // 			}
        //         }
        // 	}
        // 	apr_dir_close(dir);
        //  apr_pool_destroy(mp);

        // 相当于ls dirPath/fileName*
        fs::path filePath = fs::path(dirPath) / fileName;
        std::chrono::system_clock::time_point maxMTime{lastSaveTime};
        for (auto const &it: fs::directory_iterator{dirPath, fs::directory_options::skip_permission_denied}) {
            fs::path file = it.path();
            std::string currFileName = file.filename().string();
            // currFileName 不能等于 fileName，因此先判断一下文件名的大小
            if (currFileName.size() > fileName.size() && HasPrefix(currFileName, fileName)) {
                auto mtime = GetFileMTime(file);
#ifdef ENABLE_COVERAGE
                LogDebug("GetLastFile({}, {}, {}), cur: {}, mtime: {}, maxMTime: {}",
                         dirPath, fileName, date::format<3>(lastSaveTime), currFileName,
                         date::format<3>(mtime), date::format<3>(maxMTime));
#endif
                if (mtime > maxMTime) {
                    maxMTime = mtime;
                    filePath = file;
                }
            }
        }
        return filePath.string();
    }
}

