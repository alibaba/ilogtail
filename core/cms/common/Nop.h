//
// Created by 韩呈杰 on 2024/3/12.
//

#ifndef ARGUSAGENT_COMMON_NOP_H
#define ARGUSAGENT_COMMON_NOP_H

#include <mutex>

template<typename T>
void NopDelete(T *) {
}

template<typename T>
struct NopDeleter {
    void operator()(T *) const {
    }
};

#endif //ARGUSAGENT_COMMON_NOP_H
