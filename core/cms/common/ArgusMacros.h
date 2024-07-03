//
// Created by 韩呈杰 on 2024/5/13.
//

#ifndef ARGUSAGENT_COMMON_ARGUS_MACROS_H
#define ARGUSAGENT_COMMON_ARGUS_MACROS_H


#ifdef ENABLE_CLOUD_MONITOR

#   if !defined(WIN32) && defined(ENABLE_COVERAGE)
#       define ENABLE_FILE_CHANNEL
#   endif

#else
#   define ENABLE_FILE_CHANNEL
#endif

// # C++17开始编译器会RVO,不需要再std::move了
#if defined(__APPLE__) || defined(__FreeBSD__)
#   define RETURN_RVALUE(N) return N
#elif defined(_MSC_VER) || __cplusplus < 201703L
#   define RETURN_RVALUE(N) return std::move(N)
#else
#   define RETURN_RVALUE(N) return N
#endif

#endif // !ARGUSAGENT_COMMON_ARGUS_MACROS_H
