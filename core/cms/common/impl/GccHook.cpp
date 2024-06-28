//
// Created by 韩呈杰 on 2023/4/5.
//
/// gcc -dM -E - < /dev/null
#if defined(__GNUC__)
// doc
// ** https://www.cnblogs.com/jiangzhaowei/p/4989197.html  # 《c++异常处理（1）》
// https://gist.github.com/nkuln/2020860 # 《Override __cxa_throw and prints backtrace when exception is thrown (Linux)》
// https://zhuanlan.zhihu.com/p/478589988 # 《深入理解C++对象模型--C++异常处理》

// https://yang.observer/2021/01/22/cpp-symbol-conflict/ # 《C++静态链接符号冲突的几种处理方法》
// https://zhuanlan.zhihu.com/p/353576520 # 《编译链接时如何解决符号冲突问题》

// https://blog.csdn.net/jinking01/article/details/126728776 # 《Linux下C++中可使用的3中Hook方法》

// typedef void (*cxa_throw_type)(void *, void *, void (*)(void *));
// cxa_throw_type orig_cxa_throw = nullptr;
//
// void load_orig_throw_code() {
//     orig_cxa_throw = (cxa_throw_type) dlsym(RTLD_NEXT, "__cxa_throw");
// }
//
// extern "C" void __cxa_throw(void *thrown_exception, void *pvtinfo, void (*dest)(void *)) {
//     // printf(" ################ DETECT A THROWN !!!!! #############\n");
//     if (orig_cxa_throw == nullptr) {
//         load_orig_throw_code();
//     }
//
//     {
//         // static int throw_count = 0;
//         // void *array[10];
//         // size_t size;
//
//         // size = backtrace(array, 10);
//         // fprintf(stderr, "#### EXCEPTION THROWN (#%d) ####\n", ++throw_count);
//         // backtrace_symbols_fd(array, size, 2); // 2 == stderr
//
//         std::cerr << "exception:" << std::endl
//                   << boost::stacktrace::stacktrace() << std::flush;
//     }
//
//     orig_cxa_throw(thrown_exception, pvtinfo, dest);
// }

#include "common/GccHook.h"
#include "common/Logger.h"
#include "common/Common.h"
#include "common/ExceptionsHandler.h"

#include <unwind.h>

#include <set>

#include <boost/stacktrace.hpp>

#include "common/BoostStacktraceMt.h"

#ifndef ENABLE_COVERAGE
extern int main(int, char **);
extern void main_end();
#endif
extern "C" {
void *argus_thread_entry(void *ptr);
void argus_thread_entry_end();
}

static const auto &myFuncs = *new std::set<tagFuncPtr>{
        tagFuncPtr{(void *) safeRun, (void *) safeRunEnd},
        tagFuncPtr{(void *) argus_thread_entry, (void *) argus_thread_entry_end},
#ifndef ENABLE_COVERAGE
        tagFuncPtr{(void *)main, (void *)main_end},
#endif
};

typedef _Unwind_Reason_Code (*personality_func)(int version,
                                                _Unwind_Action actions,
                                                _Unwind_Exception_Class exception_class,
                                                struct _Unwind_Exception *ue_header,
                                                struct _Unwind_Context *context);

static personality_func gs_gcc_pf = (personality_func) dlsym(RTLD_NEXT, "__gxx_personality_v0");

// static void hook_personality_func() {
//     gs_gcc_pf = (personality_func) dlsym(RTLD_NEXT, "__gxx_personality_v0");
// }

extern "C" {
_Unwind_Reason_Code __real___gxx_personality_v0(int version,
                                                _Unwind_Action actions,
                                                _Unwind_Exception_Class exception_class,
                                                struct _Unwind_Exception *ue_header,
                                                struct _Unwind_Context *context);
} // extern "C"

static void print_call_stack() {
    using namespace common;
    using namespace boost::stacktrace;
    std::string dump_file = new_dump_file_path();
    safe_dump_to(dump_file.c_str());
    LogError("argus {}, threw unexpected exception:\n{}dump to: {}", getVersion(false),
             mt::to_string(stacktrace(1, detail::max_frames_dump)), dump_file);
}

extern "C" _Unwind_Reason_Code __wrap___gxx_personality_v0(int version,
                                                           _Unwind_Action actions,
                                                           _Unwind_Exception_Class exception_class,
                                                           struct _Unwind_Exception *ue_header,
                                                           struct _Unwind_Context *context) {
    _Unwind_Reason_Code code;
    if (gs_gcc_pf != nullptr) {
        // 动态链接stdc++
        code = gs_gcc_pf(version, actions, exception_class, ue_header, context);
    } else {
        // 静态链接stdc++
        code = __real___gxx_personality_v0(version, actions, exception_class, ue_header, context);
    }
    if (_URC_HANDLER_FOUND == code) {
        //找到了catch所有的函数

        //当前函数内的指令的地址
        void *cur_ip = (void *) (_Unwind_GetIP(context));

        // Dl_info info;
        // if (dladdr(cur_ip, &info)) {
        if (myFuncs.find(tagFuncPtr(cur_ip)) != myFuncs.end()) {
            // 当前函数是目标函数
            print_call_stack();
        }
        // }
    }

    return code;
}

#endif // !__GUNC__
