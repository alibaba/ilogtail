//
// Created by 韩呈杰 on 2023/10/25.
//

#ifndef ARGUSAGENT_MODULETOOL_H
#define ARGUSAGENT_MODULETOOL_H

struct IHandler;

#ifdef WIN32
#   define ARGUS_EXPORT
#else
#   define ARGUS_EXPORT
#endif

namespace argus {
    typedef IHandler *(*MODULE_INIT_FUNCTION)    (const char *);
    typedef int       (*MODULE_COLLECT_FUNCTION) (IHandler *, char **);
    typedef void      (*MODULE_FREE_BUF_FUNCTION)(IHandler *, const char *);
    typedef void      (*MODULE_EXIT_FUNCTION)    (IHandler *);

    //define for MODULE_SHENNONG_TYPE
    // 神农数据
    typedef int(*MODULE_SEND_FUNCTION)(const char *, double, const char *, int, const char *, const char *, int);
    // 神农基础指标（已弃用）
    typedef int(*MODULE_SEND_DATA_FUNCTION)(const char *, int);
    // 专有云（tjm）神农数据
    typedef int(*MODULE_SEND_METRIC_FUNCTION)(const char *, int, const char *, const char *, const char *);
}

#define MODULE_INIT(module)     extern "C" ARGUS_EXPORT IHandler* module##_init     (const char *args)
#define MODULE_COLLECT(module)  extern "C" ARGUS_EXPORT int       module##_collect  (IHandler *handler, char **buf)
#define MODULE_FREE_BUF(module) extern "C" ARGUS_EXPORT void      module##_free_buf (IHandler *handler, const char *buf)
#define MODULE_EXIT(module)     extern "C" ARGUS_EXPORT void      module##_exit     (IHandler *handler)
#define DECLARE_EXTERN_MODULE(module)  \
        MODULE_INIT(module);           \
        MODULE_COLLECT(module);        \
        MODULE_FREE_BUF(module);       \
        MODULE_EXIT(module)

#endif //ARGUSAGENT_MODULETOOL_H
