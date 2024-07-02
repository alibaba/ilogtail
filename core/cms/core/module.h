#ifndef ARGUS_CORE_MODULE_H
#define ARGUS_CORE_MODULE_H

#include <string>
#include <chrono>
#include "common/ModuleTool.h"

#define ENABLE_APR_SO 0

#if ENABLE_APR_SO
struct apr_pool_t;
struct apr_dso_handle_t;
#else

#include <boost/dll/shared_library.hpp>

#endif

namespace argus {
    enum ModuleType {
        MODULE_COLLECT_TYPE = 1,
        MODULE_SHENNONG_TYPE = 2,
        MODULE_NOT_SO_TYPE = 3,
    };

#include "common/test_support"
class Module {
public:
    Module() = default;
    Module(const std::string &moduleName, const std::string &name, const char *args,
           ModuleType moduleType = MODULE_COLLECT_TYPE);
    VIRTUAL ~Module();
    VIRTUAL bool Init();
    int Exit();
    int Collect(char *&buf) const;
    void FreeCollectBuf(char *&buf) const;
#if !defined(ENABLE_CLOUD_MONITOR)
    int Send(const char *m, double value, const char *id, int pid, const char *logstore, const char *instance,
             const std::chrono::system_clock::time_point &timestamp) const;
    int SendData(const char *data, size_t dataLen) const;
#endif
    int SendMetric(const char *metric, const std::chrono::system_clock::time_point &timestamp,
                   const char *values, const char *tags, const char *attrs) const;
private:
#ifdef ENABLE_CLOUD_MONITOR
    bool CloudModuleInit();
#endif
    void Close();
private:
    std::string mArgs;
    std::string mModuleName;
    std::string mName;
    std::string mModulePath;
    ModuleType mModuleType;

#if ENABLE_APR_SO
    apr_pool_t *mPool = nullptr;
    apr_dso_handle_t *mSoHandler = nullptr;
#else
    boost::dll::shared_library sharedLibrary;
#endif

    IHandler *moduleHandler = nullptr;

    MODULE_INIT_FUNCTION module_init = nullptr;
    MODULE_COLLECT_FUNCTION module_collect = nullptr;
    MODULE_FREE_BUF_FUNCTION module_free_buf = nullptr;
    MODULE_EXIT_FUNCTION module_exit = nullptr;

    MODULE_SEND_FUNCTION module_send = nullptr;
    MODULE_SEND_DATA_FUNCTION module_send_data = nullptr;
    MODULE_SEND_METRIC_FUNCTION module_send_metric = nullptr;
};
#include "common/test_support"

}
#endif