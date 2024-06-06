#include <sigar-1.6/sigar.h>
#include <iostream>

int main() {
    sigar_t *sigar;
    sigar_cpu_t cpu;

    // 初始化SIGAR库
    sigar_open(&sigar);

    // 获取CPU信息
    sigar_cpu_get(sigar, &cpu);

    // 打印CPU的一些信息
    std::cout << "CPU总使用时间: " << cpu.total << std::endl;
    std::cout << "CPU空闲时间: " << cpu.idle << std::endl;

    // 关闭SIGAR库
    sigar_close(sigar);

    return 0;
}