//
// Created by 韩呈杰 on 2023/11/6.
//

#ifndef ARGUSAGENT_SIC_SAFE_SHARED_H
#define ARGUSAGENT_SIC_SAFE_SHARED_H
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include "SafeBase.h"
#include "TypeTraitsEx.h"

// 通过拷贝构造的方式
template<typename T>
std::shared_ptr<typename std::remove_reference<T>::type> MakeCopyShared(T &&r) {
    return std::make_shared<typename std::remove_reference<T>::type>(std::forward<T>(r));
}

template<typename T, typename = void>
class SafeShared : public SafeT<std::shared_ptr<T>> {
public:
    template<typename ... Args>
    explicit SafeShared(Args &&...args) {
        this->data = std::make_shared<T>(std::forward<Args>(args)...);
    }

    using SafeT<std::shared_ptr<T>>::operator=;
    using SafeT<std::shared_ptr<T>>::Get;

    // 结果存入l，并返回
    // 返回值：true - l有变化，false - l无变化
    bool Get(std::shared_ptr<T> &l) const {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        bool diff = l.get() != this->data.get();
        if (diff) {
            l = this->data;
        }
        return diff;
    }

    std::shared_ptr<T> GetCopy() const {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return std::make_shared<T>(*this->data);
    }
};

template<typename T>
class SafeStlShared : public SafeShared<T, int> {
public:
    using SafeShared<T, int>::SafeShared;
    using SafeShared<T, int>::operator=;

    size_t Size() const {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data ? this->data->size() : 0;
    }

    bool IsEmpty() const {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return !this->data || this->data->empty();
    }
};

template<typename TMap>
std::vector<typename TMap::mapped_type> Values(const TMap &dataMap) {
    std::vector<typename TMap::mapped_type> vec;
    vec.reserve(dataMap.size());
    for (const auto &it: dataMap) {
        vec.push_back(it.second);
    }
    RETURN_RVALUE(vec);
}

template<typename TMap>
class SafeShared<TMap, typename std::enable_if<IsMap<TMap>{}>::type>
        : public SafeStlShared<TMap> {
public:
    typedef typename TMap::key_type KeyType;
    typedef typename TMap::mapped_type MappedType;

    using SafeStlShared<TMap>::SafeStlShared;
    using SafeStlShared<TMap>::operator=;

    // 查找key，并将查找结果写入target
    bool FindV(const KeyType &key, MappedType &target) const {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        auto it = this->data->find(key);
        bool found = it != this->data->end();
        if (found) {
            target = it->second;
        }
        return found;
    }

    // 查找key，并将查找结果写入target
    bool Find(const KeyType &key, TMap &target) const {
        MappedType v;
        bool found = FindV(key, v);
        if (found) {
            target.emplace(key, v);
        }
        return found;
    }

    bool Contains(const KeyType &key) const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data->find(key) != this->data->end();
    }

    std::vector<MappedType> Values() const {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return ::Values(*this->data);
    }

    template<class ...Args>
    auto Emplace(Args &&...args) -> decltype(this->data->emplace(std::forward<Args>(args)...)) {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data->emplace(std::forward<Args>(args)...);
    }
};

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Alias
template<typename K, typename V>
class SafeSharedMap: public SafeShared<std::map<K, V>> {
public:
    typedef std::map<K, V> TMap;

    using SafeShared<TMap>::SafeShared;
    using SafeShared<TMap>::operator=;
};

template<typename K, typename V>
class SafeSharedUnorderedMap: public SafeShared<std::unordered_map<K, V>> {
public:
    typedef std::unordered_map<K, V> TMap;

    using SafeShared<TMap>::SafeShared;
    using SafeShared<TMap>::operator=;
};

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shared List
template<typename TList>
class SafeShared<TList, typename std::enable_if<IsList<TList>{}>::type>
        : public SafeStlShared<TList> {
public:
    typedef typename TList::value_type ValueType;

    using SafeStlShared<TList>::SafeStlShared;
    using SafeStlShared<TList>::operator=;

    auto PushBack(const ValueType &v) -> decltype(this->data->push_back(v)) {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data->push_back(v);
    }

    template<typename ...Args>
    void EmplaceBack(Args ...args) {
        const std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data->emplace_back(std::forward<Args>(args)...);
    }
};
#endif //ARGUSAGENT_SAFE_SHARED_H
