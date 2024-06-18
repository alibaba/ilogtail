#include "CmsMain.h"

#include <iostream>
#include <apr-1/apr_version.h>
#include <sigar-1.6/sigar.h>

namespace cms {
    std::string name() {
        return "cms";
    }

    int32_t version() {
        apr_version_t version;
        apr_version(&version);
        std::cout << version.major << "." << version.minor << "." << version.patch << (version.is_dev? "[Dev]": "[Release]") << std::endl;
        return 1000000;
    }

    void cpu() {
        sigar_t *sigar;
        sigar_cpu_t cpu;

        // 初始化SIGAR库
        sigar_open(&sigar);

        // 获取CPU信息
        sigar_cpu_get(sigar, &cpu);

        std::cout << "Compiled At: " << __DATE__ << " " << __TIME__ << std::endl;

        // 打印CPU的一些信息
        std::cout << "CPU总使用时间: " << cpu.total << std::endl;
        std::cout << "CPU空闲时间: " << cpu.idle << std::endl;

        // 关闭SIGAR库
        sigar_close(sigar);
    }
}
