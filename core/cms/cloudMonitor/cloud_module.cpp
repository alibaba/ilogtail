#include "cloud_module_macros.h"
#include "cloud_module.h"

#include <map>

#include "common/ModuleTool.hpp"

struct tagModule {
    argus::MODULE_INIT_FUNCTION fnInit;
    argus::MODULE_COLLECT_FUNCTION fnCollect;
    argus::MODULE_FREE_BUF_FUNCTION fnFreeBuf;
    argus::MODULE_EXIT_FUNCTION  fnExit;
};

namespace cloudMonitor {
    DECLARE_CMS_EXTERN_MODULE(cpu);
    DECLARE_CMS_EXTERN_MODULE(memory);
    DECLARE_CMS_EXTERN_MODULE(disk);
    DECLARE_CMS_EXTERN_MODULE(net);
    DECLARE_CMS_EXTERN_MODULE(netstat);
    DECLARE_CMS_EXTERN_MODULE(system);
    DECLARE_CMS_EXTERN_MODULE(process);
    DECLARE_CMS_EXTERN_MODULE(gpu);
    DECLARE_CMS_EXTERN_MODULE(detect);
#if defined(__linux__) || defined(__unix__) || defined(WIN32)
    DECLARE_CMS_EXTERN_MODULE(xdragon);
#endif
#if defined(__linux__) || defined(__unix__)
    DECLARE_CMS_EXTERN_MODULE(tcpExt);
    DECLARE_CMS_EXTERN_MODULE(tcp);
    DECLARE_CMS_EXTERN_MODULE(rdma);
#endif

#ifdef ENABLE_COVERAGE
#define MODULE_ENTRY(X) {#X, tagModule{cloudMonitor_##X##_init, cloudMonitor_##X##_collect, cloudMonitor_##X##_free_buf, cloudMonitor_##X##_exit}}
#else
#define MODULE_ENTRY(X) {#X, tagModule{X##_init, X##_collect, X##_free_buf, X##_exit}}
#endif
    static std::map<std::string, tagModule> modules{
            MODULE_ENTRY(cpu),
            MODULE_ENTRY(memory),
            MODULE_ENTRY(disk),
            MODULE_ENTRY(net),
            MODULE_ENTRY(netstat),
            MODULE_ENTRY(system),
            MODULE_ENTRY(process),
            MODULE_ENTRY(gpu),
            MODULE_ENTRY(detect),
#if defined(__linux__) || defined(__unix__) || defined(WIN32)
            MODULE_ENTRY(xdragon),
#endif
#if defined(__linux__) || defined(__unix__)
            MODULE_ENTRY(tcpExt),
            MODULE_ENTRY(tcp),
            MODULE_ENTRY(rdma),
#endif
    };
#undef MODULE_ENTRY

    static int activeModuleCount = 0;
    bool GetModule(argus::MODULE_INIT_FUNCTION &init,
                   argus::MODULE_COLLECT_FUNCTION &collect,
                   argus::MODULE_FREE_BUF_FUNCTION &freeBuf,
                   argus::MODULE_EXIT_FUNCTION &exit,
                   const std::string &name) {
        auto it = modules.find(name);
        bool exist = it != modules.end();
        if (exist) {
            activeModuleCount++;
            init = it->second.fnInit;
            collect = it->second.fnCollect;
            freeBuf = it->second.fnFreeBuf;
            exit = it->second.fnExit;
        }
        return exist;
    }

    int GetActiveModuleCount() {
        return activeModuleCount;
    }
}
