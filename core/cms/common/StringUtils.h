/**
 * @file StringUtils
 * @author 志岩[zhiyan.czy@alibaba-inc.com]
 * @data 16-4-6 下午10:01
 * Copyright (c) 2016 Alibaba Group Holding Limited. All rights reserved.
 */


#ifndef AGENT2_STRINGUTILS_H
#define AGENT2_STRINGUTILS_H

#include <string>
#include <vector>
#include <list>
#include <set>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <algorithm>  // std::any_of
#include <cctype> // toupper

// #include <fmt/printf.h>


// C++11定义的空白符
#define SPACE_CHARS " \f\n\r\t\v"
// 6个空白符 + 尾部0
static_assert(6 + 1 == sizeof(SPACE_CHARS), "space chars unexpected");

// string stream out
class sout {
    std::ostringstream oss;
public:
    explicit sout(bool withComma = false);

    template<typename T>
    sout &operator<<(T&& r) {
        oss << std::forward<T>(r);
        return *this;
    }

    std::string str() const {
        return oss.str();
    }
};

inline bool IsEmpty(const char *s) {
    return s == nullptr || *s == '\0';
}

// 移除前后缀的空白符，
// std::string Trim(const std::string &str, bool trimLeft = true, bool trimRight = true);
// 移除前、后缀的trimCharacters中的字符
std::string Trim(const std::string &str, const std::string &trimChars, bool trimLeft = true, bool trimRight = true);

inline std::string Trim(const std::string &str, const char *trimChars, bool trimLeft = true, bool trimRight = true) {
    return trimChars ? Trim(str, std::string{trimChars}, trimLeft, trimRight) : str;
}

inline std::string TrimLeft(const std::string &str, const std::string &trimCharacters) {
    return Trim(str, trimCharacters, true, false);
}

inline std::string TrimRight(const std::string &str, const std::string &trimCharacters) {
    return Trim(str, trimCharacters, false, true);
}

inline std::string TrimSpace(const std::string &str) {
    return Trim(str, SPACE_CHARS, true, true);
}

inline std::string TrimLeftSpace(const std::string &str) {
    return TrimLeft(str, SPACE_CHARS);
}

inline std::string TrimRightSpace(const std::string &str) {
    return TrimRight(str, SPACE_CHARS);
}

std::string ToUpper(const std::string &);
std::string ToLower(const std::string &);

bool HasPrefix(const std::string &str, const std::string &prefix, bool ignoreCase = false);

template<typename Iter>
bool HasPrefixAny(const std::string &str, Iter begin, Iter end) {
    return std::any_of(begin, end, [&str](const std::string &prefix) {
        return HasPrefix(str, prefix);
    });
}

// 变参
inline bool HasPrefixAny(const std::string &str, std::initializer_list<std::string> lst) {
    return HasPrefixAny(str, lst.begin(), lst.end());
}

bool HasSuffix(const std::string &str, const std::string &suffix, bool ignoreCase = false);

template<typename Iter>
bool HasSuffixAny(const std::string &str, Iter begin, Iter end) {
    return std::any_of(begin, end, [&str](const std::string &prefix) {
        return HasSuffix(str, prefix);
    });
}

// 变参
inline bool HasSuffixAny(const std::string &str, std::initializer_list<std::string> lst) {
    return HasSuffixAny(str, lst.begin(), lst.end());
}

inline bool Contain(const std::string &str, const std::string &sub) {
    return std::string::npos != str.find(sub);
}

// equal true-完全相等， false - like
bool Contain(const std::vector<std::string> &strs, const std::string &str, bool equal = true);
bool ContainAll(const std::string &str, const std::initializer_list<std::string> &subs);
bool ContainAny(const std::string &str, const std::initializer_list<std::string> &subs);

/// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// convert between string and number
template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
T convertImpl(const std::string &str, bool hex, bool *ok = nullptr) {
    std::istringstream in(str);
    if (hex) {
        in >> std::hex;
    }

    T res;
    in >> res;
    bool success = !in.fail();
    if (ok != nullptr) {
        *ok = success;
    }
    return success ? res : 0;
}

/**
 * 将字符串转换为 unsigned long long，默认值为0
 * '10'   -> 10
 * '-10'  -> 18446744073709551606
 * '10a'  -> 10
 * '1a0'  -> 1
 * 'a10'  -> 0
 * 'ab'   -> 0
 * @param str 字符串
 * @return 转换之后的值
 */
template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
T convertHex(const std::string &str) {
    return convertImpl<T>(str, true);
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
T convert(const std::string &str) {
    return convertImpl<T>(str, false);
}

template<>
inline bool convert<bool>(const std::string &str) {
    const size_t size = str.size();
    if (size == 1) {
        char ch = str[0];
        return ch == '1' || ch == 't' || ch == 'T' || ch == 'y' || ch == 'Y';
    } else if (size > 1) {
        auto v = ToLower(str);
        return v == "true" || v == "yes" || v == "ok";
    }
    return false;
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
T convert(const char *sz) {
    return convertImpl<T>((sz == nullptr ? "" : sz), false);
}

template<>
inline bool convert<bool>(const char *sz) {
    return sz == nullptr || *sz == '\0' ? false : convert<bool>(std::string(sz));
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool convertImpl2(const std::string &str, bool hex, T &v) {
    bool ok;
    T tmp = convertImpl<T>(str, hex, &ok);
    if (ok) {
        v = tmp;
    }
    return ok;
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool convertHex(const std::string &str, T &v) {
    return convertImpl2<T>(str, true, v);
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool convert(const std::string &str, T &v) {
    return convertImpl2<T>(str, false, v);
}

inline bool convert(const std::string &str, std::string &v) {
    v = str;
    return true;
}

// 数值转字符串
template<typename T, typename std::enable_if<
        (std::is_integral<T>::value && sizeof(T) > 1) || std::is_floating_point<T>::value,
        int>::type = 0>
std::string convert(const T &n, bool withComma = false) {
    std::ostringstream oss;
    if (withComma) {
        // 如果本地化失败，则忽略
        try { oss.imbue(std::locale("")); } catch (...) {}
    }
    // 如果是double或float, set::precision可以最大限度的避免科学计数法带来的精度损失
    oss << std::setprecision(20) << n;
    return oss.str();
}

// char, unsigned char, signed char及相关变体，为避免其直接输出ascii码，此处需要转换一下
template<typename T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 1, int>::type = 0>
std::string convert(const T &n, bool withComma = false) {
    return std::numeric_limits<T>::is_signed ? convert(int(n), withComma) : convert((unsigned int) (n), withComma);
}

// 数值转十六进制字符串
template<typename T, typename std::enable_if<
        (std::is_integral<T>::value && sizeof(T) > 1) || std::is_floating_point<T>::value,
        int>::type = 0>
std::string convertHex(const T &n) {
    std::ostringstream oss;
    oss << std::hex << n;
    return oss.str();
}

// char, unsigned char, signed char及相关变体，为避免其直接输出ascii码，此处需要转换一下
template<typename T, typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 1, int>::type = 0>
std::string convertHex(const T &n) {
    return std::numeric_limits<T>::is_signed ? convertHex(int(n)) : convertHex((unsigned int) (n));
}


bool IsInt(const char *sz);

inline bool IsInt(const std::string &str) {
    return IsInt(str.c_str());
}

bool EqualIgnoreCase(const std::string &s1, const std::string &s2);

void ParseCmdLine(const std::string &cmdLine, std::vector<std::string> &args);
void ParseCmdLine(const std::string &cmdLine,
                  std::vector<std::string> &args,
                  std::vector<const char *> &argv,
                  bool withNull = true);


#ifdef WIN32
std::string GBKToUTF8(const char *szGBK);
inline std::string GBKToUTF8(const std::string &gbk) {
    return GBKToUTF8(gbk.c_str());
}

// UTF-8转GBK
std::string UTF8ToGBK(const char *szUTF8);
inline std::string UTF8ToGBK(const std::string &src) {
    return UTF8ToGBK(src.c_str());
}

std::wstring to_wstring(const char* s, size_t size = std::string::npos, unsigned int codePage = 0/*CP_ACP*/);
inline std::wstring to_wstring(const std::string& s, unsigned int codePage = 0/*CP_ACP*/) {
    return to_wstring(s.c_str(), s.size(), codePage);
}
std::string from_wstring(const wchar_t* s, size_t size = std::string::npos, unsigned int codePage = 0/*CP_ACP*/);
inline std::string from_wstring(const std::wstring& s, unsigned int codePage = 0/*CP_ACP*/) {
    return from_wstring(s.c_str(), s.size(), codePage);
}

#else
inline const char *GBKToUTF8(const char *szGBK) {
    return szGBK;
}
inline std::string GBKToUTF8(const std::string &gbk) {
    return gbk;
}
inline const char *UTF8ToGBK(const char *szUTF8) {
    return szUTF8;
}
inline std::string UTF8ToGBK(const std::string &src) {
    return src;
}
#endif

namespace common {
    using ::GBKToUTF8;

    namespace StringUtils {
        using ::EqualIgnoreCase;
        using ::ToLower;
        using ::ToUpper;
        using ::HasPrefix;
        using ::HasSuffix;
        using ::TrimSpace;

        std::string replaceAll(const std::string &origin, const std::string &from, const std::string &to);

        // 比较诡异的实现，Linux仅判断空格及制表符, Windows下仅判断空格、制表符及回车符
        // void trimInPlace(std::string &str);

        // // 仅trim空格
        // inline std::string trim(const std::string &str) {
        //     return Trim(str, " ");
        // }

        // inline std::string trimSpaceCharacter(const std::string &str) {
        //     return TrimSpace(str);
        // }
        // inline std::string trim(const std::string &str, const std::string &trimCharacters) {
        //     return Trim(str, trimCharacters);
        // }

        // 跟OneAgent冲突
        // template<typename S, typename ...Args>
        // std::string sprintf(const S &pattern, Args && ...args) {
        //     return fmt::sprintf(pattern, std::forward<Args>(args)...);
        // }

        // inline bool isNumeric(const char *sz) {
        //     return IsInt(sz);
        // }
        //
        // inline bool isNumeric(const std::string &str) {
        //     return IsInt(str);
        // }

        static inline bool StartWith(const std::string &str, const std::string &prefix) {
            return HasPrefix(str, prefix);
        }
        static inline bool EndWith(const std::string &str, const std::string &suffix) {
            return HasSuffix(str, suffix);
        }

        inline bool Contain(const std::string &str, const std::string &sub) {
            return ::Contain(str, sub);
        }

        // equal true-完全相等， false - like
        inline bool Contain(const std::vector<std::string> &strs, const std::string &str, bool equal = true) {
            return ::Contain(strs, str, equal);
        }

        using ::ContainAll;
        using ::ContainAny;

        std::string DoubleToString(double value);

        template<typename T>
        std::string join(const T &v, const std::string &splitter,
                                const std::string &open = "", const std::string &close = "",
                                bool openCloseEnabledOnNotEmpty = true); // open、close只有在v不为空的情况下才有效

        // T只接受数值型或枚举
        template<typename T, typename std::enable_if<
                std::is_integral<T>::value || std::is_floating_point<T>::value ||
                std::is_enum<T>::value, int>::type = 0>
        std::string NumberToString(const T &v) {
            std::stringstream ss;
            ss << std::setprecision(20) << v;
            return ss.str();
        }

        std::string GetTab(int n);

        // tok中的任意一个字符做为分隔
        struct SplitOpt {
            const bool bTrim;
            const bool enableEmptyLine;

            SplitOpt(bool t, bool e = false) : bTrim(t), enableEmptyLine(e) {} // NOLINT(*-explicit-constructor)
        };

        // 按chars中的任意字符分隔
        std::vector<std::string> split(const std::string &src, const std::string &chars, const SplitOpt &opt);
        // 按指定字符分隔
        inline std::vector<std::string> split(const std::string &s, char delim, const SplitOpt &opt) {
            return split(s, std::string{&delim, &delim + 1}, opt);
        }

        std::vector<std::string> splitString(const std::string &str, const std::string &sep);

        inline void ParseCmdLine(const std::string &cmdLine, std::vector<std::string> &args) {
            return ::ParseCmdLine(cmdLine, args);
        }
        inline void ParseCmdLine(const std::string &cmdLine,
                          std::vector<std::string> &args,
                          std::vector<const char *> &argv,
                          bool withNull = true) {
            return ::ParseCmdLine(cmdLine, args, argv, withNull);
        }

        template<typename T = long>
        T convertWithUnit(const std::string &str) {
            T number;
            std::string unit;
            std::istringstream{str} >> number >> unit;

            if (!unit.empty() && number != 0) {
                int ch = std::toupper(static_cast<unsigned char>(unit[0]));
                if (ch == 'K') {
                    number *= 1024;
                } else if (ch == 'M') {
                    number *= (1024 * 1024);
                } else if (ch == 'G') {
                    number *= (1024 * 1024 * 1024);
                }
            }
            return number;
        }
    }

    template<typename T>
    std::string StringUtils::join(const T &v, const std::string &splitter,
                                  const std::string &open,
                                  const std::string &close,
                                  bool openCloseEnabledOnNotEmpty) {
        std::stringstream result;

        bool openCloseEnabled = !openCloseEnabledOnNotEmpty || !v.empty();
        if (openCloseEnabled) {
            result << open;
        }

        auto it = v.begin();
        const auto end = v.end();
        if (it != end) {
            result << *it;
            for (++it; it != end; ++it) {
                result << splitter << *it;
            }
        }

        if (openCloseEnabled) {
            result << close;
        }

        return result.str();
    }
}

#endif //AGENT2_STRINGUTILS_H
