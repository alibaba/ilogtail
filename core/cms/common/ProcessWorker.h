#ifndef _PROCESS_WORKER_H_
#define _PROCESS_WORKER_H_

#include <set>
#include <memory>
#include "common/Common.h"  // pid_t

struct apr_pool_t;
struct apr_proc_t;
struct apr_procattr_t;

namespace common {

#include "test_support"
class ProcessWorker {
public:
    enum EnumWaitHow{
        WAIT,
        NOWAIT,
    };
    enum EnumExitWhy{
        PROC_EXIT = 1,          /**< process exited normally */
        PROC_SIGNAL = 2,        /**< process exited due to a signal */
        PROC_SIGNAL_CORE = 4    /**< process exited and dumped a core file */
    };

    ProcessWorker();
    VIRTUAL ~ProcessWorker();

    static bool daemonize();

    //通过sh启动
    int create(const char *prog, const char *const *args,
               const char *const *env, const char *user,
               const char *group, const char *dir, bool childBlock = false);

    VIRTUAL int create2(const char *prog, const char *const *args,
                        const char *user, int login, bool childBlock);
    //直接通过二进制启动
    VIRTUAL int create3(const char *prog, const char *const *args, const char *user,
                        const char *group, const char *dir, bool childBlock);
    VIRTUAL bool kill(int sig);
    VIRTUAL bool KillSelf(int sig) const;

    VIRTUAL bool wait(EnumWaitHow);

    VIRTUAL EnumExitWhy exitWhy() const {
        return m_exitwhy;
    }

    VIRTUAL int getExitCode() const {
        return m_exitcode;
    }

    VIRTUAL size_t input(char *buf, size_t len) const;
    VIRTUAL size_t output(char *buf, size_t len) const;
    VIRTUAL size_t errput(char *buf, size_t len) const;

    int getPid() const;

private:
    static std::set<pid_t> enumChildProcess(pid_t pid);

private:
    apr_pool_t *m_p = nullptr;
    std::shared_ptr<apr_proc_t> m_proc;
    apr_procattr_t *m_procattr = nullptr;
    EnumExitWhy m_exitwhy = PROC_EXIT;
    int m_exitcode = 0;
};
#include "test_support"

}
#endif
