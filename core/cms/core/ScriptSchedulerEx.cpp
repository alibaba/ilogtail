//
// Created by 韩呈杰 on 2024/2/28.
//
#include "ScriptSchedulerEx.h"

#ifdef ENABLE_SCRIPT_SCHEDULER_2

#include <apr-1/apr_poll.h>
#include <apr-1/apr_thread_proc.h>

#include "common/Config.h"
#include "common/CPoll.h"
#include "common/StringUtils.h"

using namespace common;
using namespace argus;

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
std::string ScriptScheduler2State::getName() const {
    return item.name;
}

std::chrono::milliseconds ScriptScheduler2State::interval() const {
    return item.scheduleIntv;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
ScriptScheduler2::ScriptScheduler2(bool enableThreadPool): SchedulerT<ScriptItem, ScriptScheduler2State>() {
    if (enableThreadPool) {
        Config *cfg = SingletonConfig::Instance();
        // getMaxProcNum: 100
        setThreadPool(ThreadPool::Option{}
                              .min(0)
                              .max(cfg->getMaxProcNum())
                              .maxIdle(30_s)
                              .name(typeName(this))
                              .makePool());
    }
}

ScriptScheduler2::~ScriptScheduler2() {
    close();
}

std::shared_ptr<ScriptScheduler2State> ScriptScheduler2::createScheduleItem(const ScriptItem &s) const {
    auto ss = std::make_shared<ScriptScheduler2State>();
    ss->item = s;

    auto nowTime = std::chrono::system_clock::now();
    auto steadyNow = std::chrono::steady_clock::now();

    // 延迟启动
    std::chrono::milliseconds factor = s.duration;
    if (factor.count() <= 0) {
        factor = SingletonConfig::Instance()->getScheduleFactor();
    }
    auto delay = getHashDuration(s.scheduleIntv, factor);
    //判断下次执行的时间是否表达式范围之内，如果不在，则需要调整t0
    if (!isTimeEffective(s, nowTime + delay)) {
        delay = decltype(delay){};  // 复位
    }
    ss->nextTime = steadyNow + delay;

    LogInfo("first run script <{}>, outputVector: {}, mid: {}, do hash to {:L}, delay: {:.3f}ms, interval={}",
            s.name, joinOutput(s.outputVector), s.mid, nowTime + delay,
            std::chrono::duration_cast<std::chrono::fraction_millis>(delay).count(), s.scheduleIntv);

    return ss;
}

class ProcPoll {
    std::string cmd;
    apr_pool_t *pool = nullptr;
    apr_pollset_t *pollSet = nullptr;
    apr_proc_t proc{};

    struct tagFdContext {
        const char *typ = nullptr;
        apr_file_t *fd = nullptr;
        std::string out;

        explicit tagFdContext(const char *s) : typ(s) {
        }

        int init(apr_pool_t *p) {
            int rv = apr_file_pipe_create_ex(&fd, &fd, APR_FULL_NONBLOCK, p);
            if (rv != APR_SUCCESS) {
                LogError("failed to create {} pipe: ({}){}", this->typ, rv, NetWorker::ErrorString(rv));
            }
            return rv;
        }

        void readPipe() {
            char buf[4096] = {0};
            apr_size_t bytes = sizeof(buf) - 1; // Leave space for null terminator
            while (apr_file_read(this->fd, buf, &bytes) == APR_SUCCESS) {
                buf[bytes] = '\0'; // Null terminate the string
                this->out.append(&buf[0]);
                bytes = sizeof(buf) - 1;
            }
        }
    } fdOut{"stdout"}, fdErr{"stderr"};

public:
    ProcPoll(const std::string &cmdline, const std::string &) {
        this->cmd = cmdline;
        apr_pool_create(&pool, nullptr);
        pollSet = Poll::createPollSet(pool, 2);
        if (!pollSet) {
            return;
        }

        // 创建管道以捕获stdout和stderr
        fdOut.init(pool);
        fdErr.init(pool);

        apr_procattr_t *procAttr = nullptr;
        // 设置进程属性
        int status = apr_procattr_create(&procAttr, pool);
        logOnFail(status, "create process attributes");
        if (status == APR_SUCCESS) {
            status = apr_procattr_io_set(procAttr, APR_FULL_BLOCK, APR_FULL_BLOCK, APR_NO_PIPE);
            logOnFail(status, "set I/O for process");
            status = apr_procattr_child_out_set(procAttr, fdOut.fd, nullptr);
            logOnFail(status, "set child stdout");
            status = apr_procattr_child_err_set(procAttr, fdErr.fd, nullptr);
            logOnFail(status, "set child stderr");
        }

        std::vector<std::string> argsCache;
        std::vector<const char *> args;
        StringUtils::ParseCmdLine(cmdline, argsCache, args, true);

        status = apr_proc_create(&proc, args[0], &args[0], nullptr, procAttr, pool);
        logOnFail(status, "create new process");
        if (status == APR_SUCCESS) {
            addToPollSet("out", fdOut);
            addToPollSet("err", fdErr);
        }
    }

    ~ProcPoll() {
        apr_file_close(fdOut.fd);
        apr_file_close(fdErr.fd);

        apr_pollset_destroy(pollSet);
        apr_pool_destroy(pool);
    }

    void wait(const std::chrono::microseconds &timeout) {
        using namespace std::chrono;

        auto endTime = steady_clock::now() + timeout;
        bool running = true;
        for (auto now = steady_clock::now(); running && now < endTime; now = steady_clock::now()) {
            const apr_pollfd_t *activePollFds = nullptr;
            apr_int32_t numActive;
            apr_interval_time_t micros = duration_cast<microseconds>(endTime - now).count();
            constexpr const apr_interval_time_t maxTimeout = 500 * 1000;
            int status = apr_pollset_poll(pollSet, std::min(maxTimeout, micros), &numActive, &activePollFds);

            if (status == APR_SUCCESS) {
                for (int i = 0; i < numActive; i++) {
                    static_cast<tagFdContext *>(activePollFds[i].client_data)->readPipe();
                }
            } else if (APR_STATUS_IS_TIMEUP(status)) {
                // 超时无活动，检查子进程状态
                apr_exit_why_e exitWhy;
                int exitCode;
                status = apr_proc_wait(&proc, &exitCode, &exitWhy, APR_NOWAIT);
                if (status != APR_CHILD_NOTDONE) {
                    // 进程已结束
                    running = false;
                    if (status == APR_SUCCESS) {
                        // 正常结束，最后一次读取管道
                        fdOut.readPipe(); // 读取stdout
                        fdErr.readPipe(); // 读取stderr
                    } else {
                        // 出错处理
                        logOnFail(status, "wait for process");
                    }
                }
            } else {
                // 出错处理
                logOnFail(status, "poll");
            }
        }
    }

    const std::string &getStdOut() const {
        return fdOut.out;
    }

    const std::string &getStdErr() const {
        return fdErr.out;
    }

private:
    void addToPollSet(const char *name, tagFdContext &ctx) {
        apr_pollfd_t pollFd{};
        pollFd.p = pool;
        pollFd.desc_type = APR_POLL_FILE;
        pollFd.reqevents = APR_POLLIN;
        pollFd.rtnevents = 0;
        pollFd.desc.f = ctx.fd;
        pollFd.client_data = (void *) &ctx;
        int rv = apr_pollset_add(pollSet, &pollFd);
        if (rv != APR_SUCCESS) {
            LogError("apr_pollset_add({}) error: ({}){}, command line: {}", name, rv, NetWorker::ErrorString(rv), cmd);
        }
    }

    void logOnFail(int rv, const char *action) {
        if (rv != APR_SUCCESS) {
            LogError("failed to {}: ({}){}, cmdline: ", action, rv, NetWorker::ErrorString(rv), this->cmd);
        }
    }
};

int ScriptScheduler2::scheduleOnce(ScriptScheduler2State &s) {
    if (!isNowEffective(s.item)) {
        return -1;
    }

    // Config *cfg = SingletonConfig::Instance();
    std::string program = s.item.collectUrl;
    program = replaceMacro(program);
    program = enableString(program);

    ProcPoll procPoll(program, s.item.scriptUser);
    procPoll.wait(s.item.timeOut);

    std::string output(!procPoll.getStdOut().empty() ? procPoll.getStdOut() : procPoll.getStdErr());


    return 0;
}

#endif // ENABLE_SCRIPT_SCHEDULER_2
