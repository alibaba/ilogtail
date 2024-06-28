#ifdef WIN32
#	include <Windows.h>
#endif

#include "module.h"
#include "common/Logger.h"
#include "common/ModuleTool.hpp"
#include "common/Config.h"
#include "common/Chrono.h"

#ifdef ENABLE_CLOUD_MONITOR

#   include "cloudMonitor/cloud_module.h"

#else
#   include "collect/module_collect.h"
#endif

#if ENABLE_APR_SO
#   include <apr-1/apr_dso.h>
#   include "common/Logger.h"
#   include "common/NetWorker.h"
#   include "common/Common.h" // typeName
#endif

using namespace std;
using namespace common;

namespace argus {
    Module::Module(const std::string &moduleName, const std::string &name, const char *args, ModuleType moduleType) {
#if defined(ENABLE_CLOUD_MONITOR)
        moduleType = MODULE_NOT_SO_TYPE;
#endif
#if ENABLE_APR_SO
        apr_pool_create(&mPool, nullptr);
#endif
        this->mModuleName = moduleName;
        this->mName = name;

        Config *cfg = SingletonConfig::Instance();
#ifdef WIN32
        this->mModulePath=(cfg->getModuleBaseDir()/(mModuleName+".dll")).string();
#else
        this->mModulePath = (cfg->getModuleBaseDir() / "lib" / (mModuleName + ".so")).string();
#endif
        this->mArgs = (args == nullptr ? "" : args);
        this->mModuleType = moduleType;
    }

    Module::~Module() {
        Exit();
#if ENABLE_APR_SO
        if (mPool) {
            apr_pool_destroy(mPool);
            mPool = nullptr;
        }
#endif
    }

#ifdef ENABLE_CLOUD_MONITOR

    bool Module::CloudModuleInit() {
        bool result = cloudMonitor::GetModule(module_init, module_collect, module_free_buf, module_exit, mModuleName);
        if (!result) {
            LogWarn("no cms-static module for {}", mModuleName);
            return false;
        }
        //call xxx_init
        this->moduleHandler = module_init(mArgs.c_str());
        if (moduleHandler != nullptr) {
            LogInfo("cms static-module({}) init success", mModuleName);
            return true;
        } else {
            //load xxx_init
            LogError("the return value of function({}_init) return nullptr", mModuleName);
            Close();
            return false;
        }
    }

#endif // ENABLE_CLOUD_MONITOR

#if ENABLE_APR_SO
    template<typename F>
    F ImportDllSymbol(apr_dso_handle_t *h, const std::string &symbolName) {
        F f = nullptr;
        apr_status_t rv = (h ? apr_dso_sym((apr_dso_handle_sym_t *) &f, h, symbolName.c_str()) : APR_ENOTIMPL);
        if(rv != APR_SUCCESS) {
            LogError("load function({}) error: ({}){}", symbolName, rv, NetWorker::ErrorString(rv));
            f = nullptr;
        }
        return f;
    }
#else

    template<typename F>
    F ImportDllSymbol(const boost::dll::shared_library &sharedLib, const std::string &symbolName) {
        F fn = nullptr;
        try {
            fn = sharedLib.get<typename std::remove_pointer<F>::type>(symbolName);
        } catch (const std::exception &ex) {
            LogError("load function({}) error: {}", symbolName, ex.what());
        }
        return fn;
    }

#endif

    IMPLEMENT_MODULE_NO_INIT(non_so_module_proxy)

    bool Module::Init() {
#ifdef ENABLE_CLOUD_MONITOR
        return CloudModuleInit();
#else
        const char *szModuleType = "static non-cms";
        if (mModuleType == MODULE_NOT_SO_TYPE) {
            moduleHandler = ModuleCollect::GetModuleCollect(mModuleName, mName, mArgs);
            if (moduleHandler == nullptr) {
                LogWarn("static non-cms module ({}): can't find, or initialize fail", mModuleName);
                return false;
            }
            module_init = nullptr;
            module_collect = non_so_module_proxy_collect;
            module_free_buf = non_so_module_proxy_free_buf;
            module_exit = non_so_module_proxy_exit;
        } else {
            szModuleType = "so";
#if ENABLE_APR_SO
            apr_status_t rv = apr_dso_load(&mSoHandler, mModulePath.c_str(), mPool);
            if (rv != APR_SUCCESS) {
                mSoHandler = nullptr;
                LogError("load so module ({}) error: ({}){}", mModulePath, rv, NetWorker::ErrorString(rv));
            }
#else
            boost::system::error_code ec;
            sharedLibrary.load(mModulePath, ec);
            if (ec.failed()) {
                LogError("load so module ({}) error: {}", mModulePath, ec.message());
            }
            const auto &mSoHandler = sharedLibrary;
#endif

            module_init = ImportDllSymbol<MODULE_INIT_FUNCTION>(mSoHandler, mModuleName + "_init");
            module_exit = ImportDllSymbol<MODULE_EXIT_FUNCTION>(mSoHandler, mModuleName + "_exit");
            if (mModuleType == MODULE_COLLECT_TYPE) {
                module_collect = ImportDllSymbol<MODULE_COLLECT_FUNCTION>(mSoHandler, mModuleName + "_collect");
                module_free_buf = ImportDllSymbol<MODULE_FREE_BUF_FUNCTION>(mSoHandler, mModuleName + "_free_buf");
            } else if (mModuleType == MODULE_SHENNONG_TYPE) {
                module_send = ImportDllSymbol<MODULE_SEND_FUNCTION>(mSoHandler, mModuleName + "_send");
                module_send_data = ImportDllSymbol<MODULE_SEND_DATA_FUNCTION>(mSoHandler, mModuleName + "_send_data");
                module_send_metric = ImportDllSymbol<MODULE_SEND_METRIC_FUNCTION>(mSoHandler, mModuleName + "_send_metric");
            }

            //call xxx_init
            if (module_init == nullptr || module_exit == nullptr
                || (mModuleType == MODULE_COLLECT_TYPE &&
                    (module_collect == nullptr || module_free_buf == nullptr))
                || (mModuleType == MODULE_SHENNONG_TYPE &&
                    (module_send == nullptr || module_send_data == nullptr || module_send_metric == nullptr))) {
                module_init = nullptr;
            }
            moduleHandler = (module_init ? module_init(mArgs.c_str()) : nullptr);
        }
        if (moduleHandler) {
            LogInfo("{} {}_init init success", szModuleType, mModuleName);
        } else {
            LogError("{} {}_init return null", szModuleType, mModuleName);
            Close();
        }
        return moduleHandler != nullptr;
#endif
    }

    void Module::Close() {
        Exit();
        // if(mSoHandler!=nullptr)
        // {
        //     apr_dso_unload(mSoHandler);
        // }
        // this->module_init=nullptr;
        // this->module_collect=nullptr;
        // this->module_exit=nullptr;
        // this->mSoHandler=nullptr;
    }

    int Module::Exit() {
        LogDebug("module({}) will exit", mModuleName);
        if (module_exit != nullptr) {
            module_exit(moduleHandler);
            moduleHandler = nullptr;
        }

        // if(mModuleCollectPtr!=nullptr)
        // {
        //     mModuleCollectPtr->Exit();
        //     delete mModuleCollectPtr;
        //     mModuleCollectPtr = nullptr;
        // }

        this->module_init = nullptr;
        this->module_collect = nullptr;
        this->module_exit = nullptr;

#if ENABLE_APR_SO
        if (mSoHandler != nullptr) {
            defer(mSoHandler = nullptr);
            apr_status_t rv = apr_dso_unload(mSoHandler);
            if (rv != APR_SUCCESS) {
                LogError("Close module ({}) error: ({}){}", mModulePath, rv, NetWorker::ErrorString(rv));
                return -1;
            }
        }
#else
        if (sharedLibrary.is_loaded()) {
            sharedLibrary.unload();
        }
#endif

        return 0;
    }

#if !defined(ENABLE_CLOUD_MONITOR)
    int Module::Send(const char *m,double value,const char *id,int pid,const char * logstore,const char *instance,
                     const std::chrono::system_clock::time_point &timestamp) const {
        if(module_send!=nullptr){
            return (*module_send)(m, value, id, pid, logstore, instance, ToSeconds<int>(timestamp));
        }else{
            LogError("module_send is null,mModuleName={}",mModuleName);
            return -1;
        }
    }

    int Module::SendData(const char *data, size_t dataLen) const {
        if (module_send_data != nullptr) {
            return (*module_send_data)(data, static_cast<int>(dataLen));
        } else {
            LogWarn("module_send_data is null, data= [{}]{}", dataLen, std::string{data, dataLen});
            return -1;
        }
    }
#endif // !ENABLE_CLOUD_MONITOR

    int Module::SendMetric(const char *metric, const std::chrono::system_clock::time_point &timestamp,
                           const char *values, const char *tags, const char *attrs) const {
        if (module_send_metric) {
            return module_send_metric(metric, ToSeconds<int>(timestamp), values, tags, attrs);
        } else {
            LogDebug("module_send_metric is null, metric={},values={},tags={},attr={}", metric, values, tags, attrs);
            return -1;
        }
    }

    int Module::Collect(char *&buf) const {
        // if (mModuleType == MODULE_COLLECT_TYPE) {
        if (module_collect != nullptr) {
            return module_collect(moduleHandler, &buf);
        } else {
            LogError("module_collect is null,mModuleName={}", mModuleName.c_str());
            return -1;
        }
        // } else if (mModuleType == MODULE_NOT_SO_TYPE) {
        //     if (mModuleCollectPtr != nullptr) {
        //         string result = mModuleCollectPtr->Collect();
        //         buf = (char *)alloc(result.size() + 1);
        //         strcpy(buf, result.c_str());
        //         return result.size();
        //         // if (result.size()+1>bufLen)
        //         // {
        //         //     LogWarn("collect result exceed bufLen({}),moduleName={}",bufLen,mModuleName.c_str());
        //         //     return -2;
        //         // }else
        //         // {
        //         //     strcpy(buf, result.c_str());
        //         //     return result.size();
        //         // }
        //     } else {
        //         LogError("mModuleCollectPtr is null,mModuleName={}", mModuleName);
        //         return -1;
        //     }
        // }
        // return 0;
    }

    void Module::FreeCollectBuf(char *&buf) const {
        if (module_free_buf && moduleHandler) {
            module_free_buf(moduleHandler, buf);
            buf = nullptr;
        }
    }
}