//
// Created by 韩呈杰 on 2023/10/26.
//

#ifndef ARGUSAGENT_MODULETOOL_HPP
#define ARGUSAGENT_MODULETOOL_HPP

#include <string>
#include <exception>
#include <memory>      // std::unique_ptr
#include "common/ThrowWithTrace.h"
#include "ModuleTool.h"
#include "Logger.h"

struct IHandler {
    virtual ~IHandler() = 0;
    virtual int Collect() = 0;
    virtual const char *Data() const = 0;
    virtual void FreeData() = 0;
};

namespace module {
    template<typename TModule>
    struct Handler : public IHandler {
        TModule m;
        std::string data;
    public:
        template<typename ...Args>
        explicit Handler(Args ...args) : m(std::forward<Args>(args)...) {
        }

        ~Handler() override = default;

        const char *Data() const override {
            return data.c_str();
        }

        void FreeData() override {
            data.clear();
        }

        int Collect() override {
            data.clear();
            return m.Collect(data);
        }
    };

    template<typename T, typename ...Args>
    Handler<T> *NewHandler(Args ...args) {
        std::unique_ptr<Handler<T>> handler(new Handler<T>());
        int ret;
        try {
            ret = handler->m.Init(std::forward<Args>(args)...);
        } catch (const std::exception &ex) {
            using namespace common;
            std::stringstream ss;
            ss << ex; // maybe include call stacks
            LogWarn("{}", ss.str());
            ret = -1;
        }
        return ret == 0 ? handler.release() : nullptr;
    }
}


#define IMPLEMENT_MODULE_NO_INIT(module)                         \
    MODULE_COLLECT(module) {                                     \
        if (buf) {                                               \
            *buf = nullptr;                                      \
        }                                                        \
        if(handler != nullptr){                                  \
            int ret = handler->Collect();                        \
            if (ret >= 0 && buf) {                               \
                *buf = const_cast<char *>(handler->Data());      \
            }                                                    \
            return ret;                                          \
        } else {                                                 \
            using namespace common;                              \
            LogError("should init module <" #module "> first");  \
            return -1;                                           \
        }                                                        \
    }                                                            \
                                                                 \
    MODULE_FREE_BUF(module) {                                    \
        if (handler != nullptr && handler->Data() == buf) {      \
            handler->FreeData();                                 \
        }                                                        \
    }                                                            \
                                                                 \
    MODULE_EXIT(module) {                                        \
        if (handler != nullptr) {                                \
            delete handler;                                      \
            handler = nullptr;                                   \
        }                                                        \
    }


#define IMPLEMENT_MODULE(module)     \
    IMPLEMENT_MODULE_NO_INIT(module) \
    MODULE_INIT(module)
// 后面跟module_init的实现代码

#endif //ARGUSAGENT_MODULETOOL_HPP
