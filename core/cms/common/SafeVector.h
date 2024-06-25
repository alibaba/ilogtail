//
// Created by 韩呈杰 on 2023/11/7.
//

#ifndef ARGUSAGENT_COMMON_SAFE_VECTOR_H
#define ARGUSAGENT_COMMON_SAFE_VECTOR_H

#include <vector>
#include <list>
#include <functional>

#include "SafeStlBase.h"
#include "common/Random.h"

template<typename T>
bool PopQueue(T &q, typename T::ValueType &data) {
    Sync(q) {
                bool has = !q.empty();
                if (has) {
                    data = q.front();
                    q.pop_front();
                }
                return has;
            }}}
}

template<typename T>
class SafePushBack : public SafeStlBase<T> {
protected:
    using SafeStlBase<T>::SafeStlBase;
public:
    typedef typename T::reference Reference;
    typedef typename T::const_reference ConstReference;
    typedef typename T::value_type ValueType;

    using SafeStlBase<T>::operator=;

    Reference Front() {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.front();
    }

    ConstReference Front() const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.front();
    }

    Reference Back() {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.back();
    }

    ConstReference Back() const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.back();
    }

    void PushBack(ValueType &&v) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.push_back(std::forward<ValueType>(v));
    }

    void PushBack(const ValueType &v) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.push_back(v);
    }

    template<typename ...Args>
    void EmplaceBack(Args &&...args) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.emplace_back(std::forward<Args>(args)...);
    }
};

template<typename T, typename TContainer = std::vector<T>>
class SafeVector : public SafePushBack<TContainer> {
public:
    typedef typename TContainer::reference Reference;
    typedef typename TContainer::const_reference ConstReference;
    typedef typename TContainer::value_type ValueType;

    using SafePushBack<TContainer>::SafePushBack;
    using SafePushBack<TContainer>::operator=;

    void Reserve(size_t n) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.reserve(n);
    }

    void Resize(size_t n) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.resize(n);
    }

    void Resize(size_t n, const T &v) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.resize(n, v);
    }

    size_t Capacity() const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.capacity();
    }

    void ShrinkToFit() {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.shrink_to_fit();
    }

    // 随机访问一个元素
    int Random(const std::function<void(int, Reference &)> &fn) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        int randomIndex = -1;
        if (!this->data.empty()) {
            randomIndex = ::Random < int > {0, static_cast<int>(this->data.size()) - 1}.next();
            fn(randomIndex, this->data.at(randomIndex));
        }
        return randomIndex;
    }

    bool At(int pos, const std::function<void(Reference &)> &fn) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        pos += (pos < 0 ? static_cast<int>(this->data.size()) : 0);
        bool ok = (0 <= pos && pos < static_cast<int>(this->data.size()));
        if (ok) {
            fn(this->data.at(pos));
        }
        return ok;
    }

    Reference At(int pos) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.at(pos < 0 ? this->data.size() + pos : pos);
    }

    ConstReference At(int pos) const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.at(pos < 0 ? this->data.size() + pos : pos);
    }

    Reference operator[](int pos) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data[pos < 0 ? this->data.size() + pos : pos];
    }

    ConstReference operator[](int pos) const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data[pos < 0 ? this->data.size() + pos : pos];
    }
};

template<typename T, typename TContainer = std::list<T>>
class SafeList : public SafePushBack<TContainer> {
public:
    bool PopFront(T *v = nullptr) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        bool notEmpty = !this->data.empty();
        if (notEmpty) {
            if (v) {
                *v = this->data.front();
            }
            this->data.pop_front();
        }
        return notEmpty;
    }
};

#endif // !ARGUSAGENT_COMMON_SAFE_VECTOR_H
