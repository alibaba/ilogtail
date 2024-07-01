/**
 * @file StringUtils
 * @author 志岩[zhiyan.czy@alibaba-inc.com]
 * @data 16-4-6 下午10:01
 * Copyright (c) 2016 Alibaba Group Holding Limited. All rights reserved.
 */
#ifdef WIN32
#	pragma warning(disable: 4819)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <Windows.h>
#endif

#include <cctype>     // std::tolower
#include <algorithm>  // std::transform
#include <functional> // std::bind, placeholders
#include <boost/algorithm/string.hpp>
#include <fmt/printf.h>
 // #ifdef WIN32
 // #   include <Windows.h>
 // #   include <string.h>  // stricmp
 // #   define strcasecmp _stricmp
 // #   define strncasecmp _strnicmp
 // #else
 // #   include <strings.h> // strcasecmp
 // #endif

#include "common/StringUtils.h"
#include "common/ArgusMacros.h"

sout::sout(bool withComma) {
    if (withComma) {
        // 允许失败
        try {
            oss.imbue(std::locale("en_US.UTF-8"));
        } catch (...) {
        }
    }
}

bool HasPrefix(const std::string &str, const std::string &prefix, bool ignoreCase) {
    return ignoreCase ?
           boost::algorithm::istarts_with(str, prefix) :
           boost::algorithm::starts_with(str, prefix);
    // return str.size() >= prefix.size() &&
    //        (ignoreCase ? strncasecmp : strncmp)(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

bool HasSuffix(const std::string &str, const std::string &suffix, bool ignoreCase) {
    return ignoreCase ?
           boost::algorithm::iends_with(str, suffix) :
           boost::algorithm::ends_with(str, suffix);
    // return str.size() >= suffix.size() &&
    //        (ignoreCase ? strcasecmp : strcmp)(str.c_str() + str.size() - suffix.size(), suffix.c_str()) == 0;
}

template<typename UnaryPredicate>
std::string trimCopy(const std::string &str, UnaryPredicate pred, bool left, bool right) {
    size_t end = str.size();
    size_t st = 0;
    const char *val = str.data();

    if (left) {
        while (st < end && pred(val[st])) {
            st++;
        }
    }
    if (right) {
        while (st < end && pred(val[end - 1])) {
            end--;
        }
    }
    return (0 < st || end < str.size()) ? str.substr(st, end - st) : str;
}

std::string Trim(const std::string &str, const std::string &trimCharacters, bool trimLeft, bool trimRight) {
    return trimCopy(str, [&](char ch) { return std::string::npos != trimCharacters.find(ch); }, trimLeft, trimRight);
}

std::string ToLower(const std::string &s) {
    return boost::to_lower_copy(s);
}

std::string ToUpper(const std::string &s) {
    return boost::to_upper_copy(s);
}

bool Contain(const std::vector<std::string> &strs, const std::string &str, bool equal) {
    std::function<bool(const std::string &)> pred;
    if (equal) {
        pred = [&str](const std::string &substr) -> bool { return str == substr; };
    } else {
        // 注意：这里是str为大串，strs[i]为子串
        pred = [&str](const std::string &substr) -> bool { return std::string::npos != str.find(substr); };
    }
    return std::any_of(strs.begin(), strs.end(), pred);
}

bool ContainAll(const std::string &str, const std::initializer_list<std::string> &subs) {
    for (const auto &it: subs) {
        if (!Contain(str, it)) {
            return false;
        }
    }
    return subs.size() > 0;
}

bool ContainAny(const std::string &str, const std::initializer_list<std::string> &subs) {
    return std::any_of(subs.begin(), subs.end(), [&](const std::string &it) { return Contain(str, it); });
}


bool IsInt(const char *sz) {
    bool ok = (sz != nullptr && *sz != '\0');
    for (auto *it = reinterpret_cast<const unsigned char *>(sz); ok && *it; ++it) {
        ok = (0 != std::isdigit(*it));
    }
    return ok;
}

//
// static void Split(const std::string &s, char sep, std::vector<std::string> &out, bool includeEmpty) {
//     std::stringstream ss;
//     ss.str(s);
//
//     // 内容不多时，优化一下
//     if (s.size() < 8 * 1024) {
//         auto number = std::count(s.begin(), s.end(), sep);
//         if (number > 0) {
//             out.reserve(number + 1);
//         }
//     }
//     std::string temp;
//     while (std::getline(ss, temp, sep)) {
//         if (includeEmpty || !temp.empty()) {
//             out.push_back(temp);
//         }
//     }
// }
//
// std::vector<std::string> Split(const std::string &s, char sep, bool includeEmpty) {
//     std::vector<std::string> words;
//     Split(s, sep, words, includeEmpty);
//     RETURN_RVALUE(words);
// }

// int CaseCmp(const char *s1, const char *s2) {
//     if (s1 == nullptr) {
//         return s2 == nullptr ? 0 : -1;
//     } else if (s2 == nullptr) {
//         return 1;
//     } else {
//         return strcasecmp(s1, s2);
//     }
// }
bool EqualIgnoreCase(const std::string &s1, const std::string &s2) {
    return boost::iequals(s1, s2);
}

/// This function provides a way to parse a generic argument string
/// into a standard argv[] form of argument list. It respects the
/// usual "whitespace" and quoteing rules. In the future this could
/// be expanded to include support for the apr_call_exec command line
/// string processing (including converting '+' to ' ' and doing the
/// url processing. It does not currently support this function.
///
///    token_context: Context from which pool allocations will occur.
///    arg_str:       Input argument string for conversion to argv[].
///    argv_out:      Output location. This is a pointer to an array
///                   of pointers to strings (ie. &(char *argv[]).
///                   This value will be allocated from the contexts
///                   pool and filled in with copies of the tokens
///                   found during parsing of the arg_str.
///
void ParseCmdLine(const std::string &cmdLine, std::vector<std::string> &argv) {
    auto skipWhitespace = [](const char *&cp) {
        while (*cp == ' ' || *cp == '\t') {
            cp++;
        }
    };

    auto checkQuotation = [](const char *&cp) {
        int isQuoted = 0;
        if (*cp == '"') {
            isQuoted = 1;
            cp++;
        } else if (*cp == '\'') {
            isQuoted = 2;
            cp++;
        }
        return isQuoted;
    };

    /* DETERMINE_NEXTSTRING:
     * At exit, cp will point to one of the following:  NULL, SPACE, TAB or QUOTE.
     * NULL implies the argument string has been fully traversed.
     */
    auto determineNextString = [](const char *&cp, int isQuoted) {
        for (; *cp != '\0'; cp++) {
            if (*cp == '\\' && (*(cp + 1) == ' ' || *(cp + 1) == '\t' ||
                                *(cp + 1) == '"' || *(cp + 1) == '\'')) {
                cp++;
                continue;
            }
            if ((!isQuoted && (*cp == ' ' || *cp == '\t'))
                || (isQuoted == 1 && *cp == '"')
                || (isQuoted == 2 && *cp == '\'')) {
                break;
            }
        }
    };

    /* REMOVE_ESCAPE_CHARS:
     * Compresses the arg string to remove all of the '\' escape chars.
     * The final argv strings should not have any extra escape chars in it.
     */
    auto removeEscapeChars = [](std::string s) {
        bool escaped = false;

        char *cleaned = const_cast<char *>(s.c_str());
        for (const char *dirty = cleaned; *dirty; dirty++) {
            if (!escaped && *dirty == '\\' && (dirty[1] == '"' || dirty[1] == '\'' || dirty[1] == '\\' || dirty[1] == ' ')) {
                escaped = true;
            } else {
                escaped = false;
                *cleaned++ = *dirty;
            }
        }
        *cleaned = 0;        /* last line of macro... */
        s.resize(cleaned - s.c_str());

        return s;
    };

    const char *cp = cmdLine.c_str();
    skipWhitespace(cp);
    const char *ct = cp;

    /* This is ugly and expensive, but if anyone wants to figure a
     * way to support any number of args without counting and
     * allocating, please go ahead and change the code.
     *
     * Must account for the trailing NULL arg.
     */
    size_t numargs = 0;
    while (*ct != '\0') {
        int isQuoted = checkQuotation(ct);
        determineNextString(ct, isQuoted);
        if (*ct != '\0') {
            ct++;
        }
        numargs++;
        skipWhitespace(ct);
    }
    argv.reserve(numargs);
    // *argv_out = apr_palloc(token_context, numargs * sizeof(char*));

    /*  determine first argument */
    for (size_t argnum = 0; argnum < numargs; argnum++) {
        skipWhitespace(cp);
        int isQuoted = checkQuotation(cp);
        ct = cp;
        determineNextString(cp, isQuoted);
        // (*argv_out)[argnum] = apr_palloc(token_context, cp - ct);
        // apr_cpystrn((*argv_out)[argnum], ct, cp - ct);
        // cleaned = dirty = (*argv_out)[argnum];
        argv.push_back(removeEscapeChars(std::string{ct, cp}));
        cp++;
    }
    // (*argv_out)[argnum] = NULL;
}

void ParseCmdLine(const std::string &cmdLine, std::vector<std::string> &args,
                               std::vector<const char *> &argv, bool withNull) {
    ParseCmdLine(cmdLine, args);

    argv.reserve(args.size() + (withNull ? 1 : 0));
    for (const std::string &s: args) {
        argv.push_back(s.c_str());
    }
    if (withNull) {
        argv.push_back(nullptr);
    }
}


#ifdef WIN32
// const char *GBK_LOCALE_NAME = "CHS";

// // GBK转UTF-8
// std::string GBKToUTF8(const char *src) {
//     std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> conv(
//             new std::codecvt<wchar_t, char, std::mbstate_t>(GBK_LOCALE_NAME));
//     std::wstring wString = conv.from_bytes(src);

//     std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
//     return convert.to_bytes(wString);
// }

// // UTF-8转GBK
// std::string UTF8ToGBK(const std::string &src) {
//     std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
//     std::wstring wString = conv.from_bytes(src);

//     std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> convert(
//             new std::codecvt<wchar_t, char, std::mbstate_t>(GBK_LOCALE_NAME));
//     return convert.to_bytes(wString);
// }

std::wstring to_wstring(const char* s, size_t size, unsigned int codePage) {
    std::wstring output;

    if (s != nullptr) {
        if (size == std::string::npos) {
            size = strlen(s);
        }
        //获取输出缓存大小
        //VC++ 默认使用ANSI，故取第一个参数为CP_ACP
        const DWORD dwFlag = CP_ACP == codePage ? MB_PRECOMPOSED : 0;
        DWORD dwBufSize = MultiByteToWideChar(codePage, dwFlag, s, (int)(size), NULL, 0);
        if (dwBufSize > 0) {
            output.resize(dwBufSize);
            MultiByteToWideChar(codePage, dwFlag, s, (int)(size), &output[0], (int)output.size());
        }
    }

    return output;
}

std::string from_wstring(const wchar_t* s, size_t size, unsigned int codePage) {
    std::string output;

    if (s != nullptr) {
        if (size == std::string::npos) {
            size = wcslen(s);
        }
        //获取输出缓存大小
        //VC++ 默认使用ANSI，故取第一个参数为CP_ACP
        const DWORD dwFlag = 0;// CP_ACP == codePage ? MB_PRECOMPOSED : 0;
        DWORD dwBufSize = WideCharToMultiByte(codePage, dwFlag, s, (int)size, NULL, 0, NULL, FALSE);
        if (dwBufSize > 0) {
            output.resize(dwBufSize);
            WideCharToMultiByte(codePage, dwFlag, s, (int)size, &output[0], (int)output.size(), NULL, FALSE);
        }
    }

    return output;
}

// GBK转UTF-8
std::string convertEncoding(const char *src, size_t size, unsigned int codePageFrom, unsigned int codePageTo) {
    std::wstring tmp = to_wstring(src, size, codePageFrom);
    return from_wstring(tmp, codePageTo);
}

std::string GBKToUTF8(const char *src) {
    return convertEncoding(src, std::string::npos, CP_ACP, CP_UTF8);
}

std::string UTF8ToGBK(const char *src) {
    return convertEncoding(src, std::string::npos, CP_UTF8, CP_ACP);
}

#endif

namespace common {

    std::string StringUtils::replaceAll(const std::string &origin, const std::string &from, const std::string &to) {
        return boost::replace_all_copy(origin, from, to);
    }

    std::vector<std::string> StringUtils::split(const std::string &src, const std::string &delim, const SplitOpt &opt) {
        std::vector<std::string> result;
        if (src.empty() || delim.empty()) {
            if (!src.empty()) {
                result = {src};
            }
        } else {
            auto pushToResult = [&](std::string tmp) {
                if (opt.bTrim) {
                    tmp = TrimSpace(tmp); // StringUtils::trimInPlace(tmp);
                }
                if (opt.enableEmptyLine || !tmp.empty()) {
                    result.push_back(tmp);
                }
            };
            size_t index, pre_index = 0;
            while ((index = src.find_first_of(delim, pre_index)) != std::string::npos) {
                pushToResult(src.substr(pre_index, index - pre_index));
                pre_index = index + 1;
            }
            if (pre_index < src.size()) {
                pushToResult(src.substr(pre_index));
            }
        }
        RETURN_RVALUE(result);
    }

    // int StringUtils::split(const std::string &src, const std::string &tokens,
    //                        std::vector<std::string> &result, const SplitOpt &opt) {
    //     if (src.empty() || tokens.empty()) {
    //         return 1;
    //     }
    //     // 换行符替换为token
    //     result = splitByAnyChar(src, (tokens != "\n" && tokens.size() == 1? tokens + "\n": tokens), opt);
    //     return 0;
    //     // if (src.empty() || tokens.empty()) {
    //     //     return 1;
    //     // }
    //     //
    //     // // 换行符替换为token
    //     // const std::string delimiter = (tokens != "\n" && tokens.size() == 1? tokens + "\n": tokens);
    //     //
    //     // auto pushToResult = [&](std::string tmp) {
    //     //     if (opt.bTrim && tokens != " ") {
    //     //         StringUtils::trimInPlace(tmp);
    //     //     }
    //     //     if (opt.enableEmptyLine || !tmp.empty()) {
    //     //         result.push_back(tmp);
    //     //     }
    //     //
    //     // };
    //     // size_t index, pre_index = 0;
    //     // while ((index = src.find_first_of(delimiter, pre_index)) != std::string::npos) {
    //     //     if (index > pre_index) {
    //     //         pushToResult(src.substr(pre_index, index - pre_index));
    //     //     }
    //     //     pre_index = index + 1;
    //     // }
    //     // if (pre_index < src.size()) {
    //     //     pushToResult(src.substr(pre_index));
    //     // }
    //     //
    //     // return 0;
    // }

    // std::vector<std::string> StringUtils::split(const std::string &s, char delim, const SplitOpt &opt) {
    //     return splitByAnyChar(s, std::string{&delim, &delim + 1}, opt);
    //     // std::stringstream ss;
    //     // ss.str(s);
    //     //
    //     // std::string item;
    //     // while (std::getline(ss, item, delim)) {
    //     //     if (opt.enableEmptyLine || !item.empty()) {
    //     //         elems.push_back(opt.bTrim? TrimSpace(item): item);
    //     //     }
    //     // }
    // }

    std::vector<std::string> StringUtils::splitString(const std::string &str, const std::string &delim) {
        std::vector<std::string> result;

        if (!str.empty() && !delim.empty()) {
            std::string::size_type pos1 = 0;
            std::string::size_type pos2 = str.find(delim);
            while (std::string::npos != pos2) {
                result.push_back(str.substr(pos1, pos2 - pos1));
                pos1 = pos2 + delim.size();
                pos2 = str.find(delim, pos1);
            }
            if (pos1 != str.length()) {
                result.push_back(str.substr(pos1));
            }
        }
        RETURN_RVALUE(result);
    }

//     void StringUtils::trimInPlace(std::string &str) {
//         if (!str.empty()) {
// #ifdef WIN32
//             static const char *trimCharacters = " \t\r";
// #else
//             static const char *trimCharacters = " \t";
// #endif
//             str = Trim(str, trimCharacters);
//         }
//     }

    // // NOTE: gcc下该函数使用时要十分小心，一但fmt与args中的数据不匹配，极易产生Segment Fault，这种错误无法捕捉，只能挂掉。
    // std::string StringUtils::vformat(const char *fmt, va_list args1) {
    //     ResourceGuard _args1Guard([&]() { va_end(args1); });
    //     // 一般情况下1K应该够用了
    //     std::string ret(1023, '\0');
    //     int actSize;
    //     {
    //         va_list args2;
    //         va_copy(args2, args1);
    //         ResourceGuard _args2Guard([&]() { va_end(args2); });
    //         actSize = std::vsnprintf(const_cast<char *>(ret.c_str()), ret.size() + 1, fmt, args2);
    //     }
    //     if (actSize >= 0) {
    //         bool expand = static_cast<size_t>(actSize) > ret.size();
    //         ret.resize(actSize);
    //         if (expand) {
    //             std::vsnprintf(const_cast<char *>(ret.c_str()), ret.size() + 1, fmt, args1);
    //         }
    //     } else {
    //         ret.clear(); // 出错了
    //     }
    //
    //     return ret;
    // }
    //
    // std::string StringUtils::format(const char *fmt, ...) {
    //     va_list args;
    //     va_start(args, fmt);
    //     return vformat(fmt, args);
    // }

    // bool StringUtils::isNumeric(const char *str) {
    //     const char *ch = str;
    //     if (str != nullptr) {
    //         for (; *ch; ch++) {
    //             if (!std::isdigit(static_cast<unsigned char>(*ch))) {
    //                 return false;
    //             }
    //         }
    //     }
    //     // 空字符串不认为是numeric的
    //     return ch > str;
    // }

    // std::string StringUtils::ToLower(const std::string &str) {
    //     return ::ToLower(str);
    // }
    //
    // std::string StringUtils::ToUpper(const std::string &str) {
    //     return ::ToUpper(str);
    // }

    std::string StringUtils::DoubleToString(double value) {
        // 网上有人推荐使用%.2g，但该方法在value数值过大时，会使用科学计数法
        std::string s = fmt::sprintf("%.2lf", value);
        auto rIt = s.rbegin();
        for (bool match{true}; match && rIt != s.rend();) {
            match = *rIt == '0';
            if (match || *rIt == '.') {
                ++rIt;
            }
        }
        s.erase(rIt.base(), s.end());
        return s;
    }

    std::string StringUtils::GetTab(int n) {
        if (n >= 0) {
            return std::string(size_t(n), '\t'); // 此处如果使用{}，会被编译器解释为两个字符
        }
        return "";
    }
}
