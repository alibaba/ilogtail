//
// Created by 韩呈杰 on 2023/11/7.
//

#ifndef ARGUSAGENT_SIC_SAFE_STL_BASE_H
#define ARGUSAGENT_SIC_SAFE_STL_BASE_H
#include "SafeBase.h"

template<typename T>
class SafeStlBase : public SafeT<T> {
protected:
    using SafeT<T>::SafeT;
public:
    using SafeT<T>::operator=;

    size_t Size() const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.size();
    }

    bool IsEmpty() const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.empty();
    }

    void Clear() {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.clear();
    }
};

#endif //ARGUSAGENT_SIC_SAFE_STL_BASE_H
