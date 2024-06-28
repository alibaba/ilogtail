#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "common/ProcessWorker.h"
#include "common/Logger.h"
#include "common/NetWorker.h"
#include "common/ExecCmd.h"
#include "common/StringUtils.h"
#include "common/ArgusMacros.h"

#include <set>

#include <apr-1/apr_file_io.h>
#include <apr-1/apr_thread_proc.h>

#if defined(__linux__) || defined(__unix__) || defined(SOLARIS)
#include <sstream>
#include <cstring>

#include <pwd.h>  // passwd
#endif


using namespace std;
using namespace common;

#define STATIC_ASSERT(N) static_assert((decltype(APR_##N))ProcessWorker::N == APR_##N, "unexpect ProcessWorker::"#N)

STATIC_ASSERT(WAIT);
STATIC_ASSERT(NOWAIT);

STATIC_ASSERT(PROC_EXIT);
STATIC_ASSERT(PROC_SIGNAL);
STATIC_ASSERT(PROC_SIGNAL_CORE);

#undef STATIC_ASSERT

ProcessWorker::ProcessWorker() {
    apr_pool_create(&m_p, nullptr);
}

ProcessWorker::~ProcessWorker() {
    apr_pool_destroy(m_p);
}

int ProcessWorker::getPid() const {
    return m_proc ? m_proc->pid : 0;
}

#define F(T, ...)                                                      \
    if ((rv = T( __VA_ARGS__ )) != APR_SUCCESS) {                      \
        LogError(#T " error, ({}){}", rv, NetWorker::ErrorString(rv)); \
        return rv;                                                     \
    }

int ProcessWorker::create(const char *prog, const char *const *args, const char *const *env,
                          const char *user, const char *group,
                          const char *dir, bool childBlock) {
#ifdef WIN32
    string name = prog;
    size_t size = name.size();
    if (size > 3 && name.substr(size - 3) == ".sh") {
        return EPERM;
    }
#endif

    apr_status_t rv;
    F(apr_procattr_create, &m_procattr, m_p)
    // if ((rv = apr_procattr_create(&m_procattr, m_p)) != APR_SUCCESS)
    // {
    //     LogError("procattr create error, return:{}", rv);
    //     return rv;
    // }

#ifdef WIN32
    int32_t in = childBlock? APR_FULL_NONBLOCK: APR_NO_PIPE;
    int32_t out = childBlock? APR_CHILD_BLOCK: APR_NO_PIPE;
    int32_t err = childBlock? APR_FULL_NONBLOCK: APR_NO_PIPE;
#else
    int32_t in = APR_FULL_NONBLOCK;
    int32_t out = childBlock ? APR_CHILD_BLOCK : APR_FULL_NONBLOCK;
    int32_t err = APR_FULL_NONBLOCK;
#endif
    F(apr_procattr_io_set, m_procattr, in, out, err)
    F(apr_procattr_cmdtype_set, m_procattr, (env ? APR_SHELLCMD : APR_SHELLCMD_ENV))
    F(apr_procattr_error_check_set, m_procattr, 1)

#ifndef WIN32
    if (user && *user) {
        F(apr_procattr_user_set, m_procattr, user, nullptr)
    }
    if (group && *group) {
        F(apr_procattr_group_set, m_procattr, group)
    }
#endif

    if (dir && *dir) {
        F(apr_procattr_dir_set, m_procattr, dir)
    }

    m_proc = std::make_shared<apr_proc_t>();
    F(apr_proc_create, m_proc.get(), prog, args, env, m_procattr, m_p)

    return rv;

    // apr_status_t rv;
    // if ((rv = apr_procattr_create(&m_procattr, m_p)) != APR_SUCCESS)
    // {
    //     LogError("procattr create error, return:{}", rv);
    //     return rv;
    // }
    //
    // if (childBlock)
    //     rv = apr_procattr_io_set(m_procattr, APR_FULL_NONBLOCK, APR_CHILD_BLOCK, APR_FULL_NONBLOCK);
    // else
    //     rv = apr_procattr_io_set(m_procattr, APR_FULL_NONBLOCK, APR_FULL_NONBLOCK, APR_FULL_NONBLOCK);
    // if (rv != APR_SUCCESS)
    // {
    //     LogError("procattr io set error, return:{}", rv);
    //     return rv;
    // }
    //
    // if (env)
    //     rv = apr_procattr_cmdtype_set(m_procattr, APR_SHELLCMD);
    // else
    //     rv = apr_procattr_cmdtype_set(m_procattr, APR_SHELLCMD_ENV);
    // if (rv != APR_SUCCESS)
    // {
    //     LogError("procattr cmdtype set error, return:{}", rv);
    //     return rv;
    // }
    //
    // if ((rv = apr_procattr_error_check_set(m_procattr, 1)) != APR_SUCCESS)
    // {
    //     LogError("procattr error check set error, return:{}", rv);
    //     return rv;
    // }
    //
    // #ifndef WIN32
    // if (user && (user[0] != 0) && (rv = apr_procattr_user_set(m_procattr, user, NULL)) != APR_SUCCESS)
    // {
    //     LogError("procattr user set error, return:{}", rv);
    //     return rv;
    // }
    //
    // if (group && (group[0] != 0) && (rv = apr_procattr_group_set(m_procattr, group)) != APR_SUCCESS)
    // {
    //     LogError("procattr group set error, return:{}", rv);
    //     return rv;
    // }
    // #endif
    //
    // if (dir && (dir[0] != 0) && (rv = apr_procattr_dir_set(m_procattr, dir)) != APR_SUCCESS)
    // {
    //     LogError("procattr dir set error, return:{}", rv);
    //     return rv;
    // }
    //
    // if ((rv = apr_proc_create(&m_proc, prog, args, env, m_procattr, m_p)) != APR_SUCCESS)
    // {
    //     LogError("proc create error, return:{}", rv);
    //     return rv;
    // }
    // return rv;
}

int ProcessWorker::create2(const char *prog, const char *const *args,
                           const char *user, int login, bool childBlock) {
    apr_status_t rv;
#if defined(WIN32) || defined(__APPLE__)
    rv = create(prog, args, nullptr, nullptr, nullptr, nullptr, childBlock);
#else
    passwd* pw = getpwnam(user);
    if (!(pw && pw->pw_name && pw->pw_name[0] && pw->pw_dir && pw->pw_dir[0] && pw->pw_passwd)) {
        return ENOENT;
    }

    int resetEnv = strstr(pw->pw_shell, "nologin")? 0: 1;
    LogDebug("pw_shell: {}, reset: {}", pw->pw_shell, resetEnv);

    F(apr_procattr_create, &m_procattr, m_p)
    // if ((rv = apr_procattr_create(&m_procattr, m_p)) != APR_SUCCESS)
    // {
    //     LogError("procattr create error, return:{}", rv);
    //     return rv;
    // }

    int32_t out = childBlock? APR_CHILD_BLOCK: APR_FULL_NONBLOCK;
    F(apr_procattr_io_set, m_procattr, APR_FULL_NONBLOCK, out, APR_FULL_NONBLOCK)
    // if (childBlock)
    //     rv = apr_procattr_io_set(m_procattr, APR_FULL_NONBLOCK, APR_CHILD_BLOCK, APR_FULL_NONBLOCK);
    // else
    //     rv = apr_procattr_io_set(m_procattr, APR_FULL_NONBLOCK, APR_FULL_NONBLOCK, APR_FULL_NONBLOCK);
    // if (rv != APR_SUCCESS)
    // {
    //     LogError("procattr io set error, return:{}", rv);
    //     return rv;
    // }

    m_proc = std::make_shared<apr_proc_t >();
    F(apr_proc_create2, m_proc.get(), prog, args, user, (resetEnv == 0? 0: login), resetEnv, m_procattr, m_p)
    // if ((rv = apr_proc_create2(&m_proc, prog, args, user, login, resetenv, m_procattr, m_p)) != APR_SUCCESS)
    // {
    //     LogError("proc create error, return:{}", rv);
    //     return rv;
    // }
#endif
    return rv;
}

int ProcessWorker::create3(const char *prog, const char *const *args, const char *user,
                           const char *group, const char *dir, bool childBlock) {
    return create(prog, args, nullptr, user, group, dir, childBlock);
    // #ifdef WIN32
    // string name = prog;
    // size_t size = name.size();
    // if (size > 3 && name.substr(size - 3) == ".sh")
    //     return EPERM;
    // #endif
    //
    // apr_status_t rv;
    // if ((rv = apr_procattr_create(&m_procattr, m_p)) != APR_SUCCESS)
    // {
    //     LogError("procattr create error, return:{}", rv);
    //     return rv;
    // }
    //
    // if (childBlock) {
    //     rv = apr_procattr_io_set(m_procattr, APR_FULL_NONBLOCK, APR_CHILD_BLOCK, APR_FULL_NONBLOCK);
    // }
    // else {
    // #ifdef WIN32
    //     rv = apr_procattr_io_set(m_procattr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE);
    // #else
    //     rv = apr_procattr_io_set(m_procattr, APR_FULL_NONBLOCK, APR_FULL_NONBLOCK, APR_FULL_NONBLOCK);
    // #endif
    // }
    // if (rv != APR_SUCCESS)
    // {
    //     LogError("procattr io set error, return:{}", rv);
    //     return rv;
    // }
    // rv = apr_procattr_cmdtype_set(m_procattr,APR_PROGRAM_ENV);
    // if (rv != APR_SUCCESS)
    // {
    //     LogError("procattr cmdtype set error, return:{}", rv);
    //     return rv;
    // }
    //
    // if ((rv = apr_procattr_error_check_set(m_procattr, 1)) != APR_SUCCESS)
    // {
    //     LogError("procattr error check set error, return:{}", rv);
    //     return rv;
    // }
    //
    // #ifndef WIN32
    // if (user && (user[0] != 0) && (rv = apr_procattr_user_set(m_procattr, user, NULL)) != APR_SUCCESS)
    // {
    //     LogError("procattr user set error, return:{}", rv);
    //     return rv;
    // }
    //
    // if (group && (group[0] != 0) && (rv = apr_procattr_group_set(m_procattr, group)) != APR_SUCCESS)
    // {
    //     LogError("procattr group set error, return:{}", rv);
    //     return rv;
    // }
    // #endif
    //
    // if (dir && (dir[0] != 0) && (rv = apr_procattr_dir_set(m_procattr, dir)) != APR_SUCCESS)
    // {
    //     LogError("procattr dir set error, return:{}", rv);
    //     return rv;
    // }
    //
    // if ((rv = apr_proc_create(&m_proc, prog, args,NULL, m_procattr, m_p)) != APR_SUCCESS)
    // {
    //     LogError("proc create error, return:{}", rv);
    //     return rv;
    // }
    // return rv;
}

bool ProcessWorker::daemonize() {
    // 脱离terminal，转为后台独立运行
    return APR_SUCCESS == apr_proc_detach(APR_PROC_DETACH_DAEMONIZE);
}

std::set<pid_t> ProcessWorker::enumChildProcess(pid_t pid) {
    ostringstream cmdStream;
#if defined (__linux__) || defined(__unix__)
    cmdStream << "pstree -pn " << pid
              << R"( | egrep -o '\([0-9]{1,10}\)' | awk -F \( '{print $2}' | awk -F \) '{print $1}')";
#elif defined(SOLARIS)
    cmdStream << "ptree " << pid << " | awk '{print $1}'";
#endif

    std::set<pid_t> pidSet;
    std::string cmd = cmdStream.str();
    if (!cmd.empty()) {
        LogDebug("enumChildProcess(pid: {}): {}", pid, cmd);
        std::vector<std::string> lines;
        ExecCmd(cmd, lines);

        for (const auto &line: lines) {
            auto p = convert<pid_t>(line);
            if (p > pid) {
                pidSet.insert(p);
            }
        }
        // char tmp[8*1024 + 1] = {0};
        // FILE *fp = popen(cmd.str().c_str(), "r");
        // if (fp != nullptr)
        // {
        //     fread(tmp, 1024, 8, fp);
        // }
        // pclose(fp);
        //
        // int len = strlen(tmp);
        // while (len > 0 && (tmp[len-1] == '\n' || tmp[len-1] == '\r' || tmp[len-1] == ' '))
        // {
        //     tmp[len-1] = 0;
        //     len--;
        // }
        //
        // int pre = 0;
        // size_t len = tmp.size(); // strlen(tmp);
        // for (int i = 0; i < len; i++)
        // {
        //     if (tmp[i] == '\n')
        //     {
        //         string s = string(tmp, pre, i - pre);
        //         int p = atoi(s.c_str());
        //         if (p > pid)
        //             pidSet.insert(p);
        //         pre = i + 1;
        //     }
        // }
        // if (pre < len)
        // {
        //     string s = string(tmp, pre, len - pre);
        //     int p = atoi(s.c_str());
        //     if (p > pid)
        //         pidSet.insert(p);
        // }
    }

    RETURN_RVALUE(pidSet);
}

bool ProcessWorker::kill(int sig) {
    bool ok = (m_proc && m_proc->pid > 1);
#if !defined(WIN32)
    if (ok) {
        std::set<pid_t> children = enumChildProcess(m_proc->pid);
        std::for_each(children.begin(), children.end(), [](pid_t p) { ::kill(p, SIGKILL); });
    }
#endif
    return ok && KillSelf(sig);
}

bool ProcessWorker::KillSelf(int sig) const {
    return !m_proc || apr_proc_kill(m_proc.get(), sig) == APR_SUCCESS;
}

bool ProcessWorker::wait(ProcessWorker::EnumWaitHow waitHow) {
    apr_status_t rv = APR_EPROC_UNKNOWN;
    if (m_proc) {
        apr_exit_why_e exitWhy = APR_PROC_EXIT;
        rv = apr_proc_wait(m_proc.get(), &m_exitcode, &exitWhy, (apr_wait_how_e) waitHow);
        m_exitwhy = (EnumExitWhy) exitWhy;
    }
    return m_proc && (rv == APR_CHILD_DONE || rv == ECHILD);
}

#define APR_FILE_WR(func, handle, buf, len) if (m_proc) { \
        func(m_proc->handle, buf, &len);                  \
    } else {                                              \
        len = 0;                                          \
    }                                                     \
    return len

size_t ProcessWorker::input(char *buf, size_t len) const {
    APR_FILE_WR(apr_file_write, in, buf, len);
}

size_t ProcessWorker::output(char *buf, size_t len) const {
    APR_FILE_WR(apr_file_read, out, buf, len);
}

size_t ProcessWorker::errput(char *buf, size_t len) const {
    APR_FILE_WR(apr_file_read, err, buf, len);
}

#undef APR_FILE_WR
