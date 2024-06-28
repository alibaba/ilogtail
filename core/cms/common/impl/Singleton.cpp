//
// Created by 韩呈杰 on 2023/11/19.
//
#include "common/Singleton.h"
#include "common/SafeMap.h"

#include <functional>
#include <list>

static auto &singletonDestroyMap = *new SafeMap<int, std::list<std::function<void()>>>{};

void registerDestroy(int flag, const std::function<void()> &fnDestroy) {
    Sync(singletonDestroyMap) {
        singletonDestroyMap[flag].push_back(fnDestroy);
    }}}
}

void destroySingletons(int flag) {
    Sync(singletonDestroyMap) {
        auto it = singletonDestroyMap.find(flag);
        if (it != singletonDestroyMap.end()) {
            for (const auto &fn: it->second) {
                fn();
            }
            singletonDestroyMap.erase(it);
        }
    }}}
}
