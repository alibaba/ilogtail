#ifndef ARGUS_COMMON_CPU_ARCH_H
#define ARGUS_COMMON_CPU_ARCH_H

// https://sourceforge.net/p/predef/wiki/OperatingSystems/ # Pre-defined Compiler Macros Wiki

#if defined(WIN32)
#	define OS_NAME "windows"
#elif defined(__APPLE__)
#	define OS_NAME "darwin"
#elif defined(__FreeBSD__)
#	define OS_NAME "freebsd"
#elif defined(__OpenBSD__)
#	define OS_NAME "openbsd"
#elif defined(__NetBSD__)
#	define OS_NAME "netbsd"
#elif defined(sun) || defined(__sun)
#   if defined(__SVR4) || defined(__svr4__)
#       define OS_NAME "solaris"
#   else
#       define OS_NAME "sunos"
#   endif
#elif defined(__ANDROID__)
#   define OS_NAME "android"
#elif defined(__linux__)
#	define OS_NAME "linux"
#elif defined(__unix__)
#   define  OS_NAME "unix"  // Linux、*BSD中也定义了该宏, MacOS中未定义
#endif

#if defined(_WIN64)
#   define CPU_ARCH "amd64"
#elif defined(WIN32) || defined(_WIN32)
#   define CPU_ARCH "386"
#elif defined(__i386__) || defined(__i386)
#   define CPU_ARCH "386"
#elif defined(__x86_64__) || defined(__x86_64)
#   define CPU_ARCH "amd64"
#elif defined(__amd64__) || defined(__AMD64__)
#   define CPU_ARCH "amd64"
#elif defined(__aarch64__) || defined(aarch64)
#   define CPU_ARCH "arm64"
#else
#   error unknown cpu architecture
#endif

#endif // ARGUS_COMMON_CPU_ARCH_H
