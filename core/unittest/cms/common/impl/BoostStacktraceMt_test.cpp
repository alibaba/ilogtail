//
// Created by 韩呈杰 on 2023/3/23.
//
#include <gtest/gtest.h>
#include <iostream>

#include <boost/filesystem.hpp>

// none-header-only (C++23才有该功能)
// with predefined macro 'BOOST_STACKTRACE_LINK', enable/disable in link time
#include <boost/stacktrace.hpp>

// header-only
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// header-only
#include <boost/format.hpp>
// with boost_url.cpp and BOOST_URL_NO_LIB defined, header-only
// #define BOOST_URL_NO_LIB // 编译时直接定义
#if !defined(DISABLE_BOOST_URL)
#include <boost/url.hpp>
#endif
// with boost_json.cpp, header-only
#include <boost/json.hpp>
// with predefined macro `BOOST_CHRONO_HEADER_ONLY`, header-only
// #define BOOST_CHRONO_VERSION 2 // 编译时直接定义
#include <boost/chrono/include.hpp>


// #   include <boost/locale.hpp> // boost::locale无法在g++ 4.9上编译
#include "common/StringUtils.h"
#include "common/BoostStacktraceMt.h"
#include "common/Logger.h"

const char *say_what(bool b) {
    return b ? "true" : "false";
}

TEST(CommonBoost, filesystem) {
    boost::filesystem::path p;
    p /= "/src/CMakeFiles/hello-world.dir/hello-world.cpp";

    std::cout << "\ncomposed path:\n";
    std::cout << "  operator<<()---------: " << p << "\n";
    std::cout << "  make_preferred()-----: " << p.make_preferred() << "\n";

    std::cout << "\nelements:\n";
    for (auto element: p)
        std::cout << "  " << element << '\n';

    std::cout << "\nobservers, native format:" << std::endl;
#ifdef BOOST_POSIX_API
    std::cout << "  native()-------------: " << p.native() << std::endl;
    std::cout << "  c_str()--------------: " << p.c_str() << std::endl;
#else // BOOST_WINDOWS_API
    std::wcout << L"  native()-------------: " << p.native() << std::endl;
    std::wcout << L"  c_str()--------------: " << p.c_str() << std::endl;
#endif
    std::cout << "  string()-------------: " << p.string() << std::endl;
    try {
        std::wcout << L"  wstring()------------: " << p.wstring() << std::endl; // 可能会由于locale的原因而导致失败
    } catch (const std::exception &ex) {
        std::wcout << std::endl;
        LogError(ex.what());
    }

    std::cout << "\nobservers, generic format:\n";
    std::cout << "  generic_string()-----: " << p.generic_string() << std::endl;
    try {
        std::wcout << L"  generic_wstring()----: " << p.generic_wstring() << std::endl;
    } catch (const std::exception &ex) {
        std::wcout << std::endl;
        LogError(ex.what());
    }

    std::cout << "\ndecomposition:\n";
    std::cout << "  root_name()----------: " << p.root_name() << '\n';
    std::cout << "  root_directory()-----: " << p.root_directory() << '\n';
    std::cout << "  root_path()----------: " << p.root_path() << '\n';
    std::cout << "  relative_path()------: " << p.relative_path() << '\n';
    std::cout << "  parent_path()--------: " << p.parent_path() << '\n';
    std::cout << "  filename()-----------: " << p.filename() << '\n';
    std::cout << "  stem()---------------: " << p.stem() << '\n';
    std::cout << "  extension()----------: " << p.extension() << '\n';

    std::cout << "\nquery:\n";
    std::cout << "  empty()--------------: " << say_what(p.empty()) << '\n';
    std::cout << "  is_absolute()--------: " << say_what(p.is_absolute()) << '\n';
    std::cout << "  has_root_name()------: " << say_what(p.has_root_name()) << '\n';
    std::cout << "  has_root_directory()-: " << say_what(p.has_root_directory()) << '\n';
    std::cout << "  has_root_path()------: " << say_what(p.has_root_path()) << '\n';
    std::cout << "  has_relative_path()--: " << say_what(p.has_relative_path()) << '\n';
    std::cout << "  has_parent_path()----: " << say_what(p.has_parent_path()) << '\n';
    std::cout << "  has_filename()-------: " << say_what(p.has_filename()) << '\n';
    std::cout << "  has_stem()-----------: " << say_what(p.has_stem()) << '\n';
    std::cout << "  has_extension()------: " << say_what(p.has_extension()) << '\n';
}

TEST(CommonBoost, uuid) {
    std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    std::cout << "uuid: " << uuid << std::endl;
    EXPECT_NE(uuid, "");
}

TEST(CommonBoost, format) {
    std::stringstream ss;
    ss << boost::format("(x,y) = (%1$+5d,%2$+5d)") % -23 % 35;

    std::cout << "boost::format: " << ss.str() << std::endl;
    EXPECT_EQ(ss.str(), "(x,y) = (  -23,  +35)");
}

#if !defined(DISABLE_BOOST_URL)
TEST(CommonBoost, url) {
    boost::url_view u(
            "https://user:pass@example.com:443/path/to/my%2dfile.txt?id=42&name=John%20Doe+Jingleheimer%2DSchmidt#page%20anchor");
    std::cout << "url       : " << u << "\n"
              << "scheme    : " << u.scheme() << "\n"
              << "authority : " << u.encoded_authority() << "\n"
              << "userinfo  : " << u.encoded_userinfo() << "\n"
              << "user      : " << u.encoded_user() << "\n"
              << "password  : " << u.encoded_password() << "\n"
              << "host      : " << u.encoded_host() << "\n"
              << "port      : " << u.port() << "\n"
              << "path      : " << u.encoded_path() << "\n"
              << "query     : " << u.encoded_query() << "\n"
              << "fragment  : " << u.encoded_fragment() << "\n";
}
#endif

TEST(CommonBoost, json) {
    const char *szJson = R"({
    "names":[
        "你好",
        123,
        {
            "sex":"female"
        },
    ],
})";
    boost::json::parse_options options;
    options.allow_trailing_commas = true;
    auto obj = boost::json::parse(szJson, boost::json::storage_ptr{}, options).as_object();
    // boost往ostream输出时，会有一个serialize的行，从而导致两边带上了双引号，此时需要再次调用c_str才可以
    std::string s = obj.if_contains("names")->as_array().at(0).as_string().c_str();
    EXPECT_EQ(s, "你好"); // utf8编码
#ifdef WIN32
    // boost::locale不能在g++ 4.9上编译
    // try {
    //     // utf8 => gbk, 便于控制台输出
    //     s = boost::locale::conv::from_utf(s.c_str(), "GBK");
    // }
    // catch (...) {
    //     std::cerr << "from_utf(\"" << s << "\") error: " << std::endl
    //         << boost::stacktrace::stacktrace();
    // }
    s = UTF8ToGBK(s);
#endif
    std::cout << "***: " << s << std::endl;
    EXPECT_EQ(123, obj.if_contains("names")->as_array().at(1).as_int64());
}

#if BOOST_VERSION >= 108100
TEST(CommonBoost, chrono) {
    std::cout << boost::chrono::symbol_format << boost::chrono::minutes{10} << '\n';
    auto now = boost::chrono::system_clock::now();
    std::cout
            << boost::chrono::time_fmt(boost::chrono::timezone::utc, "%F %T UTC") << now << std::endl
            << boost::chrono::time_fmt(boost::chrono::timezone::local, "%F %T CST") << now << std::endl;
    std::stringstream ss;
    ss << boost::chrono::time_fmt(boost::chrono::timezone::utc, "%F %T UTC") << now;
    EXPECT_EQ(ss.str()[0], '2');
}
#endif // #if BOOST_VERSION >= 108100

TEST(CommonBoost, stacktrace) {
#if defined(ONE_AGENT) || defined(__APPLE__) || defined(__FreeBSD__)
#   define __IS_CMS__
#endif
    std::stringstream ss;
    ss << boost::stacktrace::mt::to_string(boost::stacktrace::stacktrace());
#if !defined(__IS_CMS__)
    int line = __LINE__ - 2;
#endif
    std::cout << ss.str();
    EXPECT_NE(ss.str(), "");

#if defined(__IS_CMS__)
    EXPECT_NE(ss.str().find("# CommonBoost_stacktrace_Test::TestBody"), std::string::npos);
#else
#   ifdef WIN32
#       define PP
#   else
#       define PP "()"
#   endif
    std::string expected = (boost::format("# CommonBoost_stacktrace_Test::TestBody" PP " at %1%:%2%") % __FILE__ % line).str();
    std::cout << "EXPECTED: " << expected << std::endl;
    EXPECT_NE(ss.str().find(expected), std::string::npos);
#endif
#ifdef __IS_CMS__
#   undef __IS_CMS__
#endif
}
