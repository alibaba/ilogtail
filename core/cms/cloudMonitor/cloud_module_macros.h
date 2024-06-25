#ifndef ARGUSAGENT_CMS_COVERAGE_CONF_H
#define ARGUSAGENT_CMS_COVERAGE_CONF_H

#include "common/ModuleTool.hpp"

#ifdef ENABLE_COVERAGE
//单元测试云监控模块名称和集团模块名称是一样的
#   define IMPLEMENT_CMS_MODULE(module) IMPLEMENT_MODULE(cloudMonitor_ ## module)
#   define DECLARE_CMS_EXTERN_MODULE(module) DECLARE_EXTERN_MODULE(cloudMonitor_ ## module)
#else
#   define IMPLEMENT_CMS_MODULE IMPLEMENT_MODULE
#   define DECLARE_CMS_EXTERN_MODULE DECLARE_EXTERN_MODULE
#endif

#endif