#ifndef ARGUS_COMMON_SAFE_MAP_H
#define ARGUS_COMMON_SAFE_MAP_H

#include <map>
#include <unordered_map>
#include <mutex>
#include <functional>
#include "SafeStlBase.h"

// 参考: https://gitee.com/openharmony/utils_native/blob/master/base/include/safe_map.h
template<typename K, typename V, typename TMap = std::map<K, V>>
class SafeMap: public SafeStlBase<TMap> {
public:
    typedef K KeyType;
    typedef V MappedType;

    using SafeStlBase<TMap>::SafeStlBase;
    using SafeStlBase<TMap>::operator=;
    using SafeStlBase<TMap>::Set;

    // NOTE: 如果对返回值做赋值或修改操作的话，此时是无保护、非线程安全的
    const V &operator[](const K &key) const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        auto it = this->data.find(key);
        if (it != this->data.end()) {
            return it->second;
        }
        throw std::out_of_range("key not exist");
    }

    template<typename ...Args>
    bool Emplace(Args &&...args) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        auto r = this->data.emplace(std::forward<Args>(args)...);
        return r.second;
    }

    void Set(const K &key, const V &value) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data[key] = value;
    }

    bool Contains(const K &key) const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return this->data.find(key) != this->data.end();
    }

    bool Find(const K &key, V &value) const {
        bool ret = false;
        std::lock_guard<std::recursive_mutex> lock(this->mutex);

        auto iter = this->data.find(key);
        if (iter != this->data.end()) {
            value = iter->second;
            ret = true;
        }

        return ret;
    }

    bool IfExist(const K &key, const std::function<void(const V &)> &callback) const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);

        auto iter = this->data.find(key);
        auto find = iter != this->data.end();
        if (find) {
            callback(iter->second);
        }
        return find;
    }

    bool ComputeIfExist(const K &key, const std::function<V()> &fn) {
        bool ret = false;

        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        if (!this->data.empty()) {
            auto iter = this->data.find(key);
            if (iter != this->data.end()) {
                iter->second = fn();
                ret = true;
            }
        }

        return ret;
    }
    V ComputeIfAbsent(const K &key, const std::function<V()> &supplyValue) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        auto iter = this->data.find(key);
        if (iter == this->data.end()) {
            iter = this->data.emplace(key, supplyValue()).first;
        }
        return iter->second;
    }


    // 有才设置
    bool SetIfExist(const K &key, const V &value) {
        return ComputeIfExist(key, [&value]() { return value; });
    }
    V SetIfAbsent(const K &key, const V &value) {
        return ComputeIfAbsent(key, [&value]() { return value; });
    }

    // 不管有没有，都设置成value
    bool MustSet(const K &key, const V &value, V *old = nullptr) {
        bool ret = false;

        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        auto iter = this->data.find(key);
        if (iter != this->data.end()) {
            if (old) {
                *old = iter->second;
            }
            iter->second = value;
            ret = true;
        } else {
            this->data.insert(std::pair<K, V>(key, value));
        }

        return ret;
    }

    void Erase(const K &key) {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        this->data.erase(key);
    }

    TMap GetCopy() const {
        std::lock_guard<std::recursive_mutex> lock(this->mutex);
        return TMap(this->data);
    }
};

template<typename K, typename V>
class SafeUnorderedMap : public SafeMap<K, V, std::unordered_map<K, V>> {
    using SafeMap<K, V, std::unordered_map<K, V>>::SafeMap;
};

#endif // !ARGUS_COMMON_SAFE_MAP_H
