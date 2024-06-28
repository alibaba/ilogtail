//
// Created by 韩呈杰 on 2023/6/7.
//

#ifndef ARGUS_CORE_JSON_VALUE_EX_H
#define ARGUS_CORE_JSON_VALUE_EX_H

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

#include <boost/json.hpp>
#include <utility>
#include "common/StringUtils.h"

// 允许字符串和数值互转
namespace json {
    class Value;

    class Array;

    class Object;

    template<typename T>
    std::string toString(const T &v) {
        return boost::json::serialize(boost::json::value_from(v));
    }

    // Change ',' to '.' everywhere in buffer.
    // We had a sophisticated way, but it did not work in WinCE.
    // @see https://stackoverflow.com/questions/3457968/snprintf-simple-way-to-force-as-radix
    template<typename Iter>
    Iter fixNumericLocale(Iter begin, Iter end) {
        for (; begin != end; ++begin) {
            if (*begin == ',') {
                *begin = '.';
            }
        }
        return begin;
    }

    // Return iterator that would be the new end of the range [begin,end), if we
    // were to delete zeros in the end of string, but not the last zero before '.'.
    template<typename Iter>
    Iter fixZerosInTheEnd(Iter begin, Iter end) {
        for (; begin != end; --end) {
            if (*(end - 1) != '0') {
                return end;
            }
            // Don't delete the last zero before the decimal point.
            if (begin != (end - 1) && *(end - 2) == '.') {
                return end;
            }
        }
        return end;
    }

    template<typename T, typename std::enable_if<
            std::is_integral<T>::value || std::is_floating_point<T>::value,
            int>::type = 0>
    T valueToNumber(const boost::json::value *v, T def = 0) {
        if (v != nullptr) {
            if (v->is_number()) {
                def = v->to_number<T>();
            } else if (v->is_string()) {
                bool ok = false;
                T tmp = convertImpl<T>(v->as_string().c_str(), false, &ok);
                if (ok) {
                    def = tmp;
                }
            }
        }
        return def;
    }

    std::string valueToString(const boost::json::value *, const std::string & = {});

    enum class EnumPrecisionType {
        SignificantDigits = 0, ///< we set max number of significant digits in string
        DecimalPlaces          ///< we set max number of digits after "." in string
    };

    struct Pretty {
        constexpr static const int maxPrecision = 17;
        int precision = 17;
        EnumPrecisionType precisionType = EnumPrecisionType::SignificantDigits;
        bool useSpecialFloat = false;
        std::string indentation = "\t";
        std::string colon = " : ";

        void print(std::ostream &os, const boost::json::value &jv, std::string indent = {}) const;
        std::string doubleToString(double d) const;
        std::string colonSymbol() const;

        inline int actualPrecision() const {
            return precision > maxPrecision ? maxPrecision : precision;
        }
    };

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Value
    class Value {
    private:
        std::shared_ptr<boost::json::value> root;
        boost::json::value *value = nullptr;
    public:
        Value() = default;
        explicit Value(const boost::json::value &v); // 无root，仅临时封装
        Value(std::shared_ptr<boost::json::value> root, const boost::json::value *v);

        // 存在，但有可能为null值
        bool exists() const {
            return value != nullptr;
        }
        bool isNull() const {
            return !exists() || value->is_null();
        }

        std::string toString(bool emptyOnNull = true) const;
        std::string toStyledString() const;

        const boost::json::value *native() const {
            return value;
        }

        /// --------------------------
        Object asObject() const;
        bool isObject() const;

        std::string getObjectString(const std::string &k, const std::string &def = {}) const;

        template<typename T, typename std::enable_if<
                std::is_integral<T>::value || std::is_floating_point<T>::value,
                int>::type = 0>
        T getObjectNumber(const std::string &k, T def = 0) const;

        bool getObjectBool(const std::string &k, bool def = false) const;

        /// --------------------------
        Array asArray() const;
        bool isArray() const;

        /// --------------------------
        bool isString() const {
            return value != nullptr && value->is_string();
        }

        std::string asString(const std::string &def = {}) const {
            return valueToString(value, def);
        }

        bool isBool() const {
            return value != nullptr && value->is_bool();
        }

        bool asBool(bool def = false) const {
            const bool *b = (value == nullptr ? nullptr : value->if_bool());
            return b == nullptr ? def : *b;
        }

        bool isNumber() const {
            return value != nullptr && value->is_number();
        }

        template<typename T, typename std::enable_if<
                std::is_integral<T>::value || std::is_floating_point<T>::value,
                int>::type = 0>
        T asNumber(T def = 0) const {
            return valueToNumber<T>(value, def);
        }
    };

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Object
    template<typename T>
    struct Result {
        T result;
        std::string error;
        Result()= default;
        Result(const T &r, std::string err): result(r), error(std::move(err)){}
    };

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Object
    class Object {
        std::shared_ptr<boost::json::value> root;
        boost::json::object *obj = nullptr;
    public:
        Object() = default;
        Object(std::shared_ptr<boost::json::value> root, const boost::json::object *obj);
        explicit Object(const boost::json::object *o);

        bool isNull() const {
            return obj == nullptr;
        }
        size_t size() const {
            return isNull() ? 0 : obj->size();
        }
        bool empty() const {
            return isNull() || obj->empty();
        }

        // 相当于(*this)[key].asObject()
        Object getObject(const std::string &key) const;
        Array getArray(const std::string &key) const;
        Value getValue(const std::string &key) const;
        Value operator[](const std::string &key) const {
            return getValue(key);
        }

        bool contains(const std::string &key) const;

        std::string getString(const std::string &key, const std::string &def = {}) const;
        Result<std::string> getString(const std::initializer_list<std::string> &keys, const std::string &def = {}) const;
        bool getBool(const std::string &key, bool def = false) const;

        template<typename T, typename std::enable_if<
                std::is_integral<T>::value || std::is_floating_point<T>::value,
                int>::type = 0>
        T getNumber(const std::string &key, T def = 0) const {
            const T *d = this->ifContains(key, [&](const boost::json::value &v) {
                def = valueToNumber<T>(&v, def);
                return &def;
            });
            return d == nullptr ? def : *d;
        }

        template<typename T,
                typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type = 0>
        void get(const std::string &key, T &data) const {
            data = this->getNumber<T>(key, data);
        }

        size_t get(const std::string &key, std::string &data) const {
            data = this->getString(key, data);
            return data.size();
        }

        void get(const std::string &key, bool &data) const {
            data = this->getBool(key, data);
        }

        std::string toString(bool emptyOnNull = true) const;
        std::string toStyledString() const;
        // Value toValue() const; // 这个没搞定(主要原因是子数据归属问题)

        const boost::json::object *native() const {
            return obj;
        }

        json::Value rootValue() const {
            return json::Value{root, root.get()};
        }

    private:
        template<typename TFunc>
        auto ifContains(const std::string &key, TFunc fn) const -> decltype(fn(boost::json::value{})) {
            decltype(fn(boost::json::value{})) ret = nullptr;
            if (obj) {
                if (const boost::json::value *v = obj->if_contains(key)) {
                    ret = fn(std::ref(*v));
                }
            }
            return ret;
        }
    };

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Array
    class Array {
        std::shared_ptr<boost::json::value> root;
        boost::json::array *arr = nullptr;
    public:
        Array() = default;
        Array(std::shared_ptr<boost::json::value>, const boost::json::array *);
        explicit Array(const boost::json::array *a);

        bool isNull() const {
            return arr == nullptr;
        }

        size_t size() const {
            return isNull() ? 0 : arr->size();
        }

        bool empty() const {
            return isNull() || arr->empty();
        }

        // Value at(size_t pos) const;
        template<typename T>
        T at(size_t pos) const;

        Value operator[](size_t pos) const;

        const boost::json::array *native() const {
            return arr;
        }

        std::string toString(bool emptyOnNull = true) const;
        std::string toStyledString() const;

        std::vector<std::string> toStringArray() const;
        size_t toStringArray(std::vector<std::string> &) const;

        void forEach(const std::function<void(size_t index, const json::Object &)> &callback) const;
        void forEach(const std::function<void(size_t, const std::string &)> &callback) const;
        int indexOf(const std::function<bool(const Value &)> &pred) const;
    };
    template<> inline Value Array::at(size_t pos) const {
        return arr? Value{root, &(*arr)[pos]}: Value{};
    }
    template<> inline Object Array::at(size_t pos) const {
        return at<Value>(pos).asObject();
    }
    template<> inline Array Array::at(size_t pos) const {
        return at<Value>(pos).asArray();
    }
    template<> inline std::string Array::at(size_t pos) const {
        return at<Value>(pos).asString();
    }
    template<> inline bool Array::at(size_t pos) const {
        return at<Value>(pos).asBool();
    }

    template<typename T> T Array::at(size_t pos) const {
        return this->at<Value>(pos).asNumber<T>();
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    template<typename T, typename std::enable_if<
            std::is_integral<T>::value || std::is_floating_point<T>::value,
            int>::type>
    T Value::getObjectNumber(const std::string &k, T def) const {
        return asObject().getNumber<T>(k, def);
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    Value parse(const std::string &buf, std::string &error);

    static Value parse(const std::string &buf) {
        std::string error;
        return parse(buf, error);
    }

    Object parseObject(const std::string &buf, std::string &error);

    static Object parseObject(const std::string &buf) {
        std::string error;
        return parseObject(buf, error);
    }


    Array parseArray(const std::string &buf, std::string &error);

    static Array parseArray(const std::string &buf) {
        std::string error;
        return parseArray(buf, error);
    }

    std::string mergeArray(const std::string &arrayJson, const std::vector<std::string> &);
    bool GetMap(const json::Object &, const std::string &, std::unordered_map<std::string,std::string> &);
}
#endif //ARGUSAGENT_JSONVALUEEX_H
