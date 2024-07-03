#ifdef _MSC_VER
#pragma warning(disable: 4305 4307)
#endif

#include <gtest/gtest.h>
#include <climits>
#include "common/StringUtils.h"
#include "common/Logger.h"

using namespace common;
using namespace std;

TEST(CommonStringUtilsTest, IsEmpty) {
    EXPECT_TRUE(IsEmpty(nullptr));
    EXPECT_TRUE(IsEmpty(""));
    EXPECT_FALSE(IsEmpty(" "));
}

TEST(CommonStringUtilsTest, ToLower) {
    EXPECT_EQ(StringUtils::ToLower("Abc"), "abc");
    EXPECT_EQ(StringUtils::ToLower("A1bc"), "a1bc");
    EXPECT_EQ(StringUtils::ToLower("   A1bc"), "   a1bc");

    std::string s{"ABC"};
    std::string s1 = StringUtils::ToLower(s);
    EXPECT_EQ(s1, "abc");
    EXPECT_EQ(s, "ABC");
}

TEST(CommonStringUtilsTest, ToUpper) {
    EXPECT_EQ(StringUtils::ToUpper("Hello worlD"), "HELLO WORLD");
    EXPECT_EQ(StringUtils::ToUpper("Abc"), "ABC");
    EXPECT_EQ(StringUtils::ToUpper("A1bc"), "A1BC");
    EXPECT_EQ(StringUtils::ToUpper("   A1bc"), "   A1BC");

    std::string s{"abc"};
    std::string s1 = StringUtils::ToUpper(s);
    EXPECT_EQ(s1, "ABC");
    EXPECT_EQ(s, "abc");
}

TEST(CommonStringUtilsTest, EndWith) {
    EXPECT_TRUE(StringUtils::EndWith("abc", "abc"));
    EXPECT_TRUE(StringUtils::EndWith("abcd", "d"));
    EXPECT_TRUE(StringUtils::EndWith("abcdabcd", "bcd"));
    EXPECT_FALSE(StringUtils::EndWith("abcdabcd1", "bcd"));
}

TEST(CommonStringUtilsTest, ContainAllAny) {
    EXPECT_TRUE(StringUtils::ContainAll("abc", {"a"}));
    EXPECT_TRUE(StringUtils::ContainAll("abc", {"a", "c"}));
    EXPECT_FALSE(StringUtils::ContainAll("abc", {"d", "a"}));
    EXPECT_FALSE(StringUtils::ContainAll("abc", {"d", "e"}));
    EXPECT_FALSE(StringUtils::ContainAll("abc", {}));

    EXPECT_TRUE(StringUtils::ContainAny("abc", {"a"}));
    EXPECT_TRUE(StringUtils::ContainAny("abc", {"d", "a"}));
    EXPECT_FALSE(StringUtils::ContainAny("abc", {"d", "e"}));
    EXPECT_FALSE(StringUtils::ContainAny("abc", {}));
}

TEST(CommonStringUtilsTest, Contain) {
    EXPECT_FALSE(StringUtils::Contain(std::vector<std::string>{"abc"}, "a"));
    EXPECT_TRUE(StringUtils::Contain(std::vector<std::string>{"abc"}, "abc"));

    EXPECT_FALSE(Contain(std::vector<std::string>{"abc"}, "a", true));
    EXPECT_TRUE(Contain(std::vector<std::string>{"abc"}, "abc", true));
    EXPECT_TRUE(Contain(std::vector<std::string>{"abc"}, "a abc de", false));
    EXPECT_FALSE(Contain(std::vector<std::string>{"a abc de"}, "abc", false));
}

TEST(CommonStringUtilsTest, IsInt) {
    EXPECT_FALSE(IsInt(nullptr));
    EXPECT_FALSE(IsInt(""));
    EXPECT_TRUE(IsInt("0"));
    EXPECT_FALSE(IsInt("0.5"));
    EXPECT_FALSE(IsInt(std::string("0.5")));
}

TEST(CommonStringUtilsTest, split) {
    {
        vector<string> results = StringUtils::split("", "", false);
        EXPECT_TRUE(results.empty());
    }

    string test = "hello\ntest\n \n";
    vector<string> results = StringUtils::split(test, "\n", false);
    for (size_t i = 0; i < results.size(); i++) {
        cout << "i=" << i << ":" << results[i] << "end" << endl;
    }

    results = StringUtils::split(test, "\n", true);
    for (size_t i = 0; i < results.size(); i++) {
        cout << "i=" << i << ":" << results[i] << "end" << endl;
    }
}

TEST(CommonStringUtilsTest, NumberToString) {
    double before = LLONG_MAX;
    string str = StringUtils::NumberToString(before);
    std::cout << "before: " << before << ", after: " << str << std::endl;

    std::istringstream in(str);
    double after;
    in >> after;
    EXPECT_EQ(before, after);

    before = 0.000000000000000001;
    str = StringUtils::NumberToString(before);
    std::cout << "before: " << before << ", after: " << str << std::endl;

    std::istringstream in2(str);
    in2 >> after;
    EXPECT_EQ(before, after);
}

TEST(CommonStringUtilsTest, join) {
    vector<string> strs;
    string str = "abcabc";
    strs.push_back(str);
    strs.push_back(str);
    strs.push_back(str);
    {
        string result = StringUtils::join(strs, ",");
        EXPECT_EQ(result, "abcabc,abcabc,abcabc");
    }
    {
        string result = StringUtils::join(strs, "\",\"", "(\"", "\")");
        EXPECT_EQ(result, R"*(("abcabc","abcabc","abcabc"))*");
    }
    {
        string result = StringUtils::join(vector<string>(), "\",\"", "(\"", "\")");
        EXPECT_EQ(result, "");
    }
    {
        string result = StringUtils::join(vector<string>(), "\",\"", "[", "]", false);
        EXPECT_EQ(result, R"*([])*");
    }
}

TEST(CommonStringUtilsTest, convertWithUnit) {
    std::cout << "sizeof(long) => " << sizeof(long) << ", sizeof(size_t) => " << sizeof(size_t) << std::endl;
    string str("20MB");
    EXPECT_EQ(StringUtils::convertWithUnit(str), 20 * 1024 * 1024L);
    str = "20GB";
    EXPECT_EQ(StringUtils::convertWithUnit(str), 20971520 * 1024L);
    str = "20K";
    EXPECT_EQ(StringUtils::convertWithUnit(str), 20 * 1024L);
    str = "20A";
    EXPECT_EQ(StringUtils::convertWithUnit(str), 20L);
    str = "A";
    EXPECT_EQ(StringUtils::convertWithUnit(str), 0L);
    str = "12A";
    EXPECT_EQ(StringUtils::convertWithUnit(str), 12L);
    str = "A12";
    EXPECT_EQ(StringUtils::convertWithUnit(str), 0L);
}

TEST(CommonStringUtilTest, split2) {
    string testStr1("  aa bb cc   dd    ee  ");
    vector<string> words = StringUtils::split(testStr1, ' ', false);
    EXPECT_EQ(words.size(), size_t(5));
    if (words.size() > 4) {
        EXPECT_EQ(words[0], "aa");
        EXPECT_EQ(words[1], "bb");
        EXPECT_EQ(words[2], "cc");
        EXPECT_EQ(words[3], "dd");
        EXPECT_EQ(words[4], "ee");
    }
    string testStr2(",,aa,,bb,cc,,,dd,,,,,ee");

    words = StringUtils::split(testStr2, ',', false);
    if (words.size() > 4) {
        EXPECT_EQ(words[0], "aa");
        EXPECT_EQ(words[1], "bb");
        EXPECT_EQ(words[2], "cc");
        EXPECT_EQ(words[3], "dd");
        EXPECT_EQ(words[4], "ee");
    }
}

// TEST(CommonStringUtilTest, trimInPlace) {
//     {
//         string str1 = " \t123 ";
//         string str2 = "123";
//         StringUtils::trimInPlace(str1);
//         EXPECT_EQ(str2, str1);
//     }
// #ifdef WIN32
//     {
//         string str1 = " \t123 \r";
//         string str2 = "123";
//         StringUtils::trimInPlace(str1);
//         EXPECT_EQ(str2, str1);
//
//     }
// #endif
// }

TEST(CommonStringUtilTest, trimSpaceCharacter) {
    string str1 = " \t\v1 23 \r\t\n";
    string str2 = "1 23";
    std::string str3 = StringUtils::TrimSpace(str1);
    EXPECT_NE(str2, str1);
    EXPECT_EQ(str2, str3);
}

TEST(CommonStringUtilTest, format) {
    {
        std::string s = fmt::sprintf(std::string("%d"), 22222);
        EXPECT_EQ(size_t(5), s.size());
        EXPECT_EQ(s, "22222");
    }
    {
        std::string s1(4096, '1');
        EXPECT_EQ(s1.size(), size_t(4096));
        std::string s = fmt::sprintf("]]] %s", s1.c_str());
        EXPECT_EQ(s.size(), s1.size() + 4);
        EXPECT_EQ(s.substr(s.size() - 5, 5), "11111");
        EXPECT_EQ(s.substr(0, 4), "]]] ");
        EXPECT_EQ(s1.substr(0, 5), s.substr(4, 5));
    }
    {
        std::string s = fmt::sprintf("");
        EXPECT_TRUE(s.empty());
    }
    {
        std::string s = fmt::sprintf("%s", "");
        EXPECT_TRUE(s.empty());
    }
    // std::cout << "-------------------------" << std::endl;
    // {
    //     std::string s = fmt::sprintf("%b", 0);
    //     cout << "s: " << s << endl;
    //     EXPECT_TRUE(s.empty());
    // }
}

TEST(CommonStringUtilTest, IsNumeric) {
    // StringUtils::isNumber
    EXPECT_TRUE (IsInt("123"));
    EXPECT_TRUE (IsInt(std::string("123")));
    EXPECT_FALSE(IsInt(""));
    EXPECT_FALSE(IsInt(nullptr));
    EXPECT_FALSE(IsInt(" "));
    EXPECT_FALSE(IsInt("1 "));
}

TEST(CommonStringUtilTest, DoubleToString) {
    EXPECT_EQ(StringUtils::DoubleToString(1.23), "1.23");
    EXPECT_EQ(StringUtils::DoubleToString(1.0), "1");
    EXPECT_EQ(StringUtils::DoubleToString(0.2), "0.2");
}

TEST(CommonStringUtilTest, SplitString) {
    {
        auto lines = StringUtils::splitString("123\n456", "\n");
        EXPECT_EQ(size_t(2), lines.size());
        EXPECT_EQ(lines.at(0), "123");
        EXPECT_EQ(lines.at(1), "456");
    }
    {
        auto lines = StringUtils::splitString("123\n", "\n");
        EXPECT_EQ(size_t(1), lines.size());
        EXPECT_EQ(lines.at(0), "123");
    }
    {
        auto lines = StringUtils::splitString("\n456", "\n");
        EXPECT_EQ(size_t(2), lines.size());
        EXPECT_EQ(lines.at(0), "");
        EXPECT_EQ(lines.at(1), "456");
    }
    {
        auto lines = StringUtils::splitString("456", "\n");
        EXPECT_EQ(size_t(1), lines.size());
        EXPECT_EQ(lines.at(0), "456");
    }
    {
        auto lines = StringUtils::splitString("", "\n");
        EXPECT_TRUE(lines.empty());
    }
}

TEST(CommonStringUtilTest, ParseCmdLine) {
    {
        std::vector<std::string> argv;
        StringUtils::ParseCmdLine("a.exe", argv);
        EXPECT_EQ(argv.size(), size_t(1));
        EXPECT_EQ(argv[0], "a.exe");
    }
    {
        std::vector<std::string> argv;
        StringUtils::ParseCmdLine("a.exe 'abc'", argv);
        EXPECT_EQ(argv.size(), size_t(2));
        EXPECT_EQ(argv[0], "a.exe");
        EXPECT_EQ(argv[1], "abc");
    }
    {
        std::vector<std::string> argv;
        StringUtils::ParseCmdLine(R"("C:\Program Files\a.exe" 'abc')", argv);
        EXPECT_EQ(argv.size(), size_t(2));
        EXPECT_EQ(argv[0], "C:\\Program Files\\a.exe");
        EXPECT_EQ(argv[1], "abc");
    }
}

TEST(CommonStringUtilTest, EqualIgnoreCase) {
    EXPECT_TRUE(StringUtils::EqualIgnoreCase("A", "a"));
    EXPECT_TRUE(StringUtils::EqualIgnoreCase("A", "A"));
    EXPECT_TRUE(StringUtils::EqualIgnoreCase("", ""));
}


TEST(CommonStringUtilTest, HasPrefix) {
    EXPECT_TRUE(HasPrefix("123", "12"));
    EXPECT_TRUE(HasPrefix("123", ""));
    EXPECT_FALSE(HasPrefix("123", "1234"));
    EXPECT_FALSE(HasPrefix("123", "a"));

    EXPECT_TRUE(HasPrefixAny("123", {"a", "12"}));
    EXPECT_FALSE(HasPrefixAny("123", {}));

    std::vector<std::string> prefixes{"a", "12"};
    EXPECT_TRUE(HasPrefixAny("123", prefixes.begin(), prefixes.end()));

    EXPECT_TRUE(HasPrefix("1Abced", "1aB", true));
}

TEST(CommonStringUtilTest, HasSuffix) {
    EXPECT_TRUE(HasSuffix("123", "23"));
    EXPECT_TRUE(HasSuffix("123", ""));
    EXPECT_FALSE(HasSuffix("123", "1234"));
    EXPECT_FALSE(HasSuffix("123", "a"));

    EXPECT_TRUE(HasSuffixAny("123", {"a", "23"}));
    EXPECT_FALSE(HasSuffixAny("123", {}));

    std::vector<std::string> prefixes{"a", "23"};
    EXPECT_TRUE(HasSuffixAny("123", prefixes.begin(), prefixes.end()));

    EXPECT_TRUE(HasSuffix("1Abced", "CeD", true));
}

TEST(CommonStringUtilTest, TrimStr1) {

    {
        std::string expected = "123";
        std::string str = " \t123\r\n";
        std::string actual = TrimSpace(str);
        EXPECT_EQ(expected, actual);
        // str未受影响
        EXPECT_EQ(std::string{" \t123\r\n"}, str);
    }
    {
        std::string str = " \t123\r\n";
        std::string expected = "123\r\n";
        std::string actual = TrimLeftSpace(str);
        EXPECT_EQ(expected, actual);
        // str未受影响
        EXPECT_EQ(std::string{" \t123\r\n"}, str);
    }
    {
        std::string str = " \t123\r\n";
        std::string expected = " \t123";
        std::string actual = TrimRightSpace(str);
        EXPECT_EQ(expected, actual);
        // str未受影响
        EXPECT_EQ(std::string{" \t123\r\n"}, str);
    }
    {
        std::string str = " \t123\r\n";
        std::string expected = "\t123\r\n";
        std::string actual = TrimLeft(str, " ");
        EXPECT_EQ(expected, actual);
        // str未受影响
        EXPECT_EQ(std::string{" \t123\r\n"}, str);
    }
    {
        std::string str = " \t123\r\n";
        std::string expected = " \t123";
        std::string actual = TrimRight(str, SPACE_CHARS);
        EXPECT_EQ(expected, actual);
        // str未受影响
        EXPECT_EQ(std::string{" \t123\r\n"}, str);
    }
    {
        std::string str = " \t123\r\n ";
        std::string expected = " \t123\r";
        std::string actual = TrimRight(str, " \n");
        EXPECT_EQ(expected, actual);
        // str未受影响
        EXPECT_EQ(std::string{" \t123\r\n "}, str);
    }
    {
        std::string str = "  \t\n\v";
        std::string actual = TrimSpace(str);
        EXPECT_EQ(actual, "");
        EXPECT_NE(str, "");
    }
}

TEST(CommonStringUtilTest, TrimStr2) {
    std::string str;
    std::string expected;

    std::string actual = TrimSpace(str);
    EXPECT_EQ(expected, actual);
}

TEST(CommonStringUtilTest, TrimSpecialChar) {
    std::string str = "123abccba";
    std::string expected = "123";

    std::string actual = Trim(str, "abc");
    EXPECT_EQ(expected, actual);
}

TEST(CommonStringUtilTest, SpaceChars) {
    for (const char *ch = SPACE_CHARS; *ch; ++ch) {
        EXPECT_TRUE(std::isspace(static_cast<unsigned char>(*ch)));
    }
}

TEST(CommonStringUtilTest, ToLower) {
    EXPECT_EQ(ToLower("Abc"), "abc");
    EXPECT_EQ(ToLower("A1bc"), "a1bc");
    EXPECT_EQ(ToLower("   A1bc"), "   a1bc");

    std::string s{"ABC"};
    std::string s1 = ToLower(s);
    EXPECT_EQ(s1, "abc");
    EXPECT_EQ(s, "ABC"); // 确保s未改变
}

TEST(CommonStringUtilTest, ToUpper) {
    EXPECT_EQ(ToUpper("Abc"), "ABC");
    EXPECT_EQ(ToUpper("A1bc"), "A1BC");
    EXPECT_EQ(ToUpper("   A1bc"), "   A1BC");

    std::string s{"abc"};
    std::string s1 = ToUpper(s);
    EXPECT_EQ(s1, "ABC");
    EXPECT_EQ(s, "abc");
}


TEST(CommonStringUtilTest, ConvertStringToUnsignedLong) {
    EXPECT_EQ(0UL, convert<unsigned long>("0"));
    EXPECT_EQ(static_cast<unsigned long>(-1), convert<unsigned long>("-1"));
}

TEST(CommonStringUtilTest, ConvertHexStringToUnsignedLong) {
    EXPECT_EQ(0UL, convertHex<unsigned long>("0"));
    EXPECT_EQ(15UL, convertHex<unsigned long>("F"));

    int v = -1;
    EXPECT_FALSE(convertHex<int>("Z", v));
    EXPECT_EQ(-1, v);

    EXPECT_TRUE(convertHex<int>("a", v));
    EXPECT_EQ(10, v);
}

TEST(CommonStringUtilTest, ConvertStringToInt) {
    EXPECT_FALSE(convert<bool>((const char *) nullptr));

    EXPECT_FALSE(convert<bool>(""));
    EXPECT_FALSE(convert<bool>("0"));
    EXPECT_TRUE(convert<bool>("1"));
    EXPECT_TRUE(convert<bool>("T"));
    EXPECT_TRUE(convert<bool>("t"));
    EXPECT_TRUE(convert<bool>("Y"));
    EXPECT_TRUE(convert<bool>("y"));

    EXPECT_TRUE(convert<bool>("true"));
    EXPECT_TRUE(convert<bool>("tRuE"));
    EXPECT_TRUE(convert<bool>("Yes"));
    EXPECT_TRUE(convert<bool>("yes"));
    EXPECT_TRUE(convert<bool>("OK"));
    EXPECT_TRUE(convert<bool>("ok"));

    EXPECT_EQ(0, convert<int>("0"));
    EXPECT_EQ(-1, convert<int>("-1"));
}

TEST(CommonStringUtilTest, ConvertRealToString) {
    EXPECT_EQ("0", convert(0));
    EXPECT_EQ("-1", convert(-1));

    EXPECT_EQ("97", convert('a'));
    EXPECT_EQ("97", convert(char('a')));
    EXPECT_EQ("97", convert(int8_t('a')));
    EXPECT_EQ("97", convert(uint8_t('a')));

    EXPECT_EQ("97", convert(wchar_t('a')));
    EXPECT_EQ("97", convert(int16_t('a')));
    EXPECT_EQ("97", convert(uint16_t('a')));

    EXPECT_EQ("1234567890.1240000725", convert(1234567890.124));
}

TEST(CommonStringUtilTest, ConvertHexStringToInt) {
    EXPECT_EQ(0, convertHex<int>("0"));
    EXPECT_EQ(127, convertHex<int>("7F"));

    EXPECT_EQ("61", convertHex('a'));
    EXPECT_EQ("61", convertHex(char('a')));
    EXPECT_EQ("61", convertHex(int8_t('a')));
    EXPECT_EQ("61", convertHex(uint8_t('a')));
    EXPECT_EQ("61", convertHex(wchar_t('a')));
    EXPECT_EQ("61", convertHex(int16_t('a')));
    EXPECT_EQ("61", convertHex(uint16_t('a')));
}

TEST(CommonStringUtilTest, Split) {
    {
        auto words = StringUtils::split("A", ' ', false);
        EXPECT_EQ(std::vector<std::string>{"A"}, words);
    }
    {
        auto words = StringUtils::split("A     B", ' ', false);
        EXPECT_EQ(size_t(2), words.size());
        EXPECT_EQ("A", words[0]);
        EXPECT_EQ("B", words[1]);

    }
    {
        // 2个空格
        auto words = StringUtils::split("A  B", ' ', {false, true});
        EXPECT_EQ(size_t(3), words.size());
        EXPECT_EQ("A", words[0]);
        EXPECT_EQ("", words[1]);
        EXPECT_EQ("B", words.at(2));
    }
}

TEST(CommonStringUtilTest, SoutWithComma) {
    bool localeEnabled = true;
    try {
        std::locale("en_US.UTF-8");
    } catch (...) {
        localeEnabled = false;
    }
    std::string s = (sout{true} << 1000).str();
    const char *expect = (localeEnabled? "1,000": "1000");
    LogInfo("{} => {}", 1000, s);
    EXPECT_EQ(s, expect);
}
