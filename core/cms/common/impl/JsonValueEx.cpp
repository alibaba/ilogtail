//
// Created by 韩呈杰 on 2023/6/7.
//
#include "common/JsonValueEx.h"
#include "common/ArgusMacros.h"

#include <cmath>
#include <utility>

namespace json {
    Value parse(const std::string &s, std::string &error) {
        boost::system::error_code ec;
        boost::json::parse_options opt;
        opt.allow_comments = true;
        opt.allow_trailing_commas = true;
        opt.allow_invalid_utf8 = true;  // this increases parsing performance

        Value ret;
        boost::json::value jv = boost::json::parse(s, ec, boost::json::storage_ptr(), opt);
        if (ec.failed()) {
            error = ec.message();
        } else {
            ret = Value{std::make_shared<boost::json::value>(std::move(jv)), nullptr};
        }

        return ret;
    }

    Object parseObject(const std::string &s, std::string &error) {
        Object obj;
        Value jv = parse(s, error);
        if (error.empty()) {
            obj = jv.asObject();
            if (obj.isNull()) {
                error = "not a json object";
            }
        }
        return obj;
    }

    Array parseArray(const std::string &s, std::string &error) {
        Array arr;
        Value jv = parse(s, error);
        if (error.empty() && !jv.isNull()) {
            arr = jv.asArray();
            if (arr.isNull()) {
                error = "not a json array";
            }
        }
        return arr;
    }

    std::string valueToString(const boost::json::value *v, const std::string &def) {
        std::string ret{def};
        if (v != nullptr) {
            if (auto *s = v->if_string()) {
                ret = s->c_str();
            } else if (auto *i = v->if_int64()) {
                ret = convert(*i);
            } else if (auto *u = v->if_uint64()) {
                ret = convert(*u);
            } else if (auto *d = v->if_double()) {
                ret = Pretty{}.doubleToString(*d);
            }
        }
        return ret;
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Pretty

    std::string Pretty::doubleToString(double d) const {
        std::string buf;
        if (!std::isfinite(d)) {
            if (useSpecialFloat) {
                buf = (std::isnan(d) ? "nan" : (d < 0 ? "-inf" : "inf")); // special floats
            } else {
                buf = (std::isnan(d) ? "null" : (d < 0 ? "-1e+9999" : "1e+9999"));
            }
            return buf;
        }

        buf.resize(48);
        while (true) {
            int len = snprintf((char *) buf.data(), buf.size() + 1,
                               (precisionType == EnumPrecisionType::SignificantDigits ? "%.*g" : "%.*f"),
                               actualPrecision(), d);
            bool overflow = len > static_cast<int>(buf.size());
            buf.resize(len);
            if (!overflow) {
                break;
            }
        }
        buf.erase(fixNumericLocale(buf.begin(), buf.end()), buf.end());

        // strip the zero padding from the right
        if (precisionType == EnumPrecisionType::DecimalPlaces) {
            buf.erase(fixZerosInTheEnd(buf.begin(), buf.end()), buf.end());
        }

        // try to ensure we preserve the fact that this was given to us as a double on input
        if (buf.find('.') == std::string::npos && buf.find('e') == std::string::npos) {
            buf += ".0";
        }

        // os << std::setprecision(precision > 17? 17: precision) << jv.get_double();
        return buf;
    }

    std::string Pretty::colonSymbol() const {
        return (this->indentation.empty() ? ":" : this->colon);
    }

    boost::json::object sortKey(const boost::json::object &obj) {
        // cppjson把object按key排序了
        std::vector<boost::json::string_view> names;
        names.reserve(obj.size());
        for (auto const &it: obj) {
            names.push_back(it.key());
        }
        std::sort(names.begin(), names.end());
        boost::json::object _obj;
        for (const auto &key: names) {
            _obj[key] = obj.at(key);
        }
        return _obj;
    }

    void Pretty::print(std::ostream &os, const boost::json::value &jv, std::string indent) const {
        switch (jv.kind()) {
            case boost::json::kind::object: {
                auto const &obj = jv.get_object();
                if (obj.empty()) {
                    os << "{}";
                } else {
                    os << "{";
                    indent.append(this->indentation);
                    if (!obj.empty()) {
                        const char *newLine = "\n";
                        // const auto &_obj = obj;
                        boost::json::object _obj = sortKey(obj);
                        for (auto const &it: _obj) {
                            os << newLine << indent << boost::json::serialize(it.key()) << colonSymbol();
                            this->print(os, it.value(), indent);
                            newLine = ",\n";
                        }
                    }
                    os << "\n";
                    indent.resize(indent.size() - this->indentation.size());
                    os << indent << "}";
                }
                break;
            }

            case boost::json::kind::array: {
                auto const &arr = jv.get_array();
                if (arr.empty()) {
                    os << "[]";
                } else {
                    os << "[";
                    indent.append(this->indentation);
                    if (!arr.empty()) {
                        const char *newLine = "\n";
                        auto it = arr.begin();
                        for (;;) {
                            os << newLine << indent;
                            this->print(os, *it, indent);
                            if (++it == arr.end()) {
                                break;
                            }
                            newLine = ",\n";
                            // os << ",\n";
                        }
                    }
                    os << "\n";
                    indent.resize(indent.size() - this->indentation.size());
                    os << indent << "]";
                }
                break;
            }

            case boost::json::kind::string:
                os << boost::json::serialize(jv.get_string());
                break;

            case boost::json::kind::uint64:
                os << jv.get_uint64();
                break;

            case boost::json::kind::int64:
                os << jv.get_int64();
                break;

            case boost::json::kind::double_:
                os << doubleToString(jv.get_double());
                break;

            case boost::json::kind::bool_:
                os << (jv.get_bool() ? "true" : "false");
                break;

            case boost::json::kind::null:
                os << "null";
                break;
        }

        if (indent.empty()) {
            os << "\n";
        }
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Value
    Value::Value(const boost::json::value &v) {
        value = const_cast<boost::json::value *>(&v);
    }

    Value::Value(std::shared_ptr<boost::json::value> r, const boost::json::value *v) {
        this->root = std::move(r);
        value = const_cast<boost::json::value *>(v == nullptr ? root.get() : v);
    }

    std::string Value::toString(bool emptyOnNull) const {
        return value ? boost::json::serialize(*value) : (emptyOnNull ? std::string{} : std::string{"null"});
    }

    std::string Value::toStyledString() const {
        std::stringstream ss;
        if (value) {
            Pretty{}.print(ss, *value);
        }
        return TrimRightSpace(ss.str());
    }

    Object Value::asObject() const {
        auto *p = const_cast<boost::json::object *>(value == nullptr ? nullptr : value->if_object());
        return Object{root, p};
    }

    bool Value::isObject() const {
        return value != nullptr && value->is_object();
    }

    std::string Value::getObjectString(const std::string &k, const std::string &def) const {
        return asObject().getString(k, def);
    }

    bool Value::getObjectBool(const std::string &k, bool def) const {
        return asObject().getBool(k, def);
    }

    Array Value::asArray() const {
        auto *p = const_cast<boost::json::array *>(value == nullptr ? nullptr : value->if_array());
        return Array{root, p};
    }

    bool Value::isArray() const {
        return value != nullptr && value->is_array();
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Object
    Object::Object(const boost::json::object *d) {
        obj = const_cast<boost::json::object *>(d);
    }

    Object::Object(std::shared_ptr<boost::json::value> r, const boost::json::object *d) : Object(d) {
        root = std::move(r);
    }

    bool Object::contains(const std::string &key) const {
        return obj != nullptr && obj->contains(key);
    }

    std::string Object::getString(const std::string &key, const std::string &def) const {
        std::string ret{def};
        ifContains(key, [&](const boost::json::value &v) {
            ret = valueToString(&v, def);
            return &ret;
        });
        return ret;
    }

    Result<std::string> Object::getString(const std::initializer_list<std::string> &keys, const std::string &d) const {
        if (keys.size() == 0) {
            return {std::string{}, "getString({}) error: no keys"};
        }

        std::string path;
        const char *sep = "";

        boost::json::object *nodeObj = obj;
        boost::json::value *node = nullptr;
        for (auto const &it: keys) {
            if (nodeObj == nullptr) {
                return Result<std::string>{d, "node not object: " + path};
            }

            path.append(sep).append(it);
            sep = ".";

            node = nodeObj->if_contains(it);
            if (node == nullptr) {
                return Result<std::string>{d, "path not exist: " + path};
            }
            nodeObj = node->if_object();
        }

        return {valueToString(node, d), std::string{}};
    }

    bool Object::getBool(const std::string &key, const bool def) const {
        const bool *b = ifContains(key, [](const boost::json::value &v) { return v.if_bool(); });
        return b != nullptr ? *b : def;
    }

    Object Object::getObject(const std::string &key) const {
        typedef boost::json::object type;
        const type *b = ifContains(key, [](const boost::json::value &v) { return v.if_object(); });
        return Object{root, const_cast<type *>(b)};
    }

    Array Object::getArray(const std::string &key) const {
        typedef boost::json::array type;
        const type *b = ifContains(key, [](const boost::json::value &v) { return v.if_array(); });
        return Array{root, const_cast<type *>(b)};
    }

    Value Object::getValue(const std::string &key) const {
        const boost::json::value *v = ifContains(key, [](const boost::json::value &v) { return &v; });
        return v != nullptr ? Value{root, v} : Value{};
    }

    std::string Object::toString(bool emptyOnNull) const {
        return obj ? boost::json::serialize(*obj) : (emptyOnNull ? std::string{} : std::string{"null"});
    }

    std::string Object::toStyledString() const {
        std::stringstream oss;
        if (obj) {
            Pretty{}.print(oss, boost::json::value_from(*obj));
        }
        return TrimRightSpace(oss.str());
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Array
    Array::Array(std::shared_ptr<boost::json::value> r, const boost::json::array *d) : Array(d) {
        root = std::move(r);
    }

    Array::Array(const boost::json::array *a) {
        arr = const_cast<boost::json::array *>(a);
    }

    Value Array::operator[](size_t pos) const {
        Value ret;
        if (arr) {
            ret = Value{root, &(*arr)[pos]};
        }
        return ret;
    }

    std::string Array::toString(bool emptyOnNull) const {
        return arr ? boost::json::serialize(*arr) : (emptyOnNull ? std::string{} : std::string{"null"});
    }

    std::string Array::toStyledString() const {
        std::stringstream oss;
        if (arr) {
            Pretty{}.print(oss, boost::json::value_from(*arr));
        }
        return TrimRightSpace(oss.str());
    }

    size_t Array::toStringArray(std::vector<std::string> &vec) const {
        vec.reserve(vec.size() + size());

        size_t count = 0;
        forEach([&](size_t, const std::string &s) {
            count++;
            vec.push_back(s);
        });

        return count;
    }

    std::vector<std::string> Array::toStringArray() const {
        std::vector<std::string> vec;
        toStringArray(vec);
        RETURN_RVALUE(vec);
    }

    void Array::forEach(const std::function<void(size_t, const json::Object &)> &callback) const {
        const size_t count = size();
        for (size_t i = 0; i < count; i++) {
            json::Object obj = at<Object>(i);
            if (!obj.isNull()) {
                callback(i, obj);
            }
        }
    }

    void Array::forEach(const std::function<void(size_t, const std::string &)> &callback) const {
        if (arr) {
            size_t index = 0;
            for (const auto &it: *arr) {
                if (auto *s = it.if_string()) {
                    callback(index++, s->c_str());
                }
            }
        }
    }

    int Array::indexOf(const std::function<bool(const Value &)> &pred) const {
        if (arr) {
            int index = -1;
            for (const auto &it: *arr) {
                index++;
                if (pred(Value{root, &it})) {
                    return index;
                }
            }
        }
        return -1;
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// utility
    std::string mergeArray(const std::string &arrayJson, const std::vector<std::string> &data) {
        std::string s = arrayJson;
        if (!data.empty()) {
            json::Array arr = parseArray(arrayJson.empty() ? "[]" : arrayJson);
            if (auto *native = const_cast<boost::json::array *>(arr.native())) {
                native->reserve(native->size() + data.size());
                for (const auto &it: data) {
                    Value value = parse(it);
                    if (value.isObject()) {
                        native->push_back(*value.native());
                    }
                }
                s = arr.toStyledString();
            }
        }
        return s;
    }

    bool GetMap(const json::Object &root, const std::string &key,
                    std::unordered_map<std::string, std::string> &output) {
        json::Object data = root.getObject(key);
        if (!data.isNull()) {
            for (const auto &it: *data.native()) {
                auto *s = it.value().if_string();
                output[it.key()] = (s ? s->c_str() : std::string{});
            }
        }
        return !data.isNull();
    }

}
