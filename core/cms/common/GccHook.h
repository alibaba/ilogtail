//
// Created by 韩呈杰 on 2023/4/6.
//
#if defined(__GNUC__)

#ifndef ARGUSAGENT_MODULESCHEDULER_GCC_H
#define ARGUSAGENT_MODULESCHEDULER_GCC_H

#include <pthread.h>
#include <set>

struct tagFuncPtr {
    const void *begin;
    const void *end;

    tagFuncPtr(const void *l, const void *r) : begin(l), end(r) {
    }

    explicit tagFuncPtr(const void *addr) : begin(addr), end(addr) {
    }

    bool operator<(const tagFuncPtr &r) const {
        return this->end < r.begin || (this->end == r.begin && this->begin < r.begin);
    }
};

#endif //ARGUSAGENT_MODULESCHEDULER_GCC_H
#endif // __GUNC__