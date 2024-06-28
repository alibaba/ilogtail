#ifndef ARGUS_CMS_CLOUD_MODULE_H
#define ARGUS_CMS_CLOUD_MODULE_H

#include <string>
#include "common/ModuleTool.h"

namespace cloudMonitor {
    bool GetModule(argus::MODULE_INIT_FUNCTION &init,
                   argus::MODULE_COLLECT_FUNCTION &collect,
                   argus::MODULE_FREE_BUF_FUNCTION &freeBuf,
                   argus::MODULE_EXIT_FUNCTION &exit,
                   const std::string &name);
    int GetActiveModuleCount();
}

#endif