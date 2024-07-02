//
// Created by 韩呈杰 on 2023/6/23.
//
#include <gtest/gtest.h>
#include "common/RegexUtil.h"

using namespace common;

#if defined(__linux__) || defined(__APPLE__)

#include <regex.h>

static int matchFirst(const std::string &input, const std::string &pattern, std::string &out) {
    regex_t reg;
    regmatch_t pm[1];
    out = "";
    int iret = regcomp(&reg, pattern.c_str(), REG_EXTENDED | REG_NEWLINE);
    if (iret != 0) {
        return -1;
    }
    iret = regexec(&reg, input.c_str(), 1, pm, 0);
    if (iret == REG_NOMATCH) {
        out = "";
        return -3;
    } else if (iret != 0) {
        return -2;
    } else {
        out = input.substr(pm[0].rm_so, pm[0].rm_eo - pm[0].rm_so);
        iret = pm[0].rm_eo;
    }
    regfree(&reg);

    return iret;
}

#endif

TEST(RegexUtilTest, MatchFirst_InvalidPattern) {
    const char *line = "Roses are #ff0001";
    const char *pattern = "#([a-f0-9]{2}";

    std::string output;
    int ret = RegexUtil::match_first(line, pattern, output);
    EXPECT_EQ(-1, ret);

    EXPECT_FALSE(RegexUtil::match(line, pattern));
}

TEST(RegexUtilTest, MatchFirst_NoMatch) {
    const char *line = "Roses are #ff0001";
    const char *pattern = "(abc)";

    std::string output;
    int ret = RegexUtil::match_first(line, pattern, output);
    EXPECT_EQ(-3, ret);

    EXPECT_FALSE(RegexUtil::match(line, pattern));
}

TEST(RegexUtilTest, MatchFirst) {
    const char *line = "Roses are #ff0001";
    const char *pattern = "#([a-f0-9]{2})([a-f0-9]{2})([a-f0-9]{2})";

    {
        std::string output;
        int ret = RegexUtil::match_first(line, pattern, output);
        EXPECT_EQ(17, ret);
        EXPECT_EQ(output, "#ff0001");

        EXPECT_TRUE(RegexUtil::match(line, pattern));
    }
#if defined(__linux__) || defined(__APPLE__)
    {
        std::string output;
        int ret = matchFirst(line, pattern, output);
        EXPECT_EQ(17, ret);
        EXPECT_EQ(output, "#ff0001");
    }
#endif
}

TEST(RegexUtilTest, MatchOnly) {
    EXPECT_TRUE(RegexUtil::match("1", "^[0-9]+$"));
    EXPECT_TRUE(RegexUtil::match("12", "^[0-9]+$"));

    EXPECT_FALSE(RegexUtil::match(" 1", "^[0-9]+$"));
    EXPECT_FALSE(RegexUtil::match(" 12", "^[0-9]+$"));
}

TEST(RegexUtilTest, MatchFetch) {
    {
        std::vector<std::string> ret;
        EXPECT_EQ(1, RegexUtil::match("1", "^([0-9]+)$", 1, ret));
        EXPECT_EQ(size_t(1), ret.size());
        EXPECT_EQ(ret[0], "1");
    }
    {
        std::vector<std::string> ret;
        EXPECT_EQ(1, RegexUtil::match("12", "^([0-9]+)$", 1, ret));
        EXPECT_EQ(size_t(1), ret.size());
        EXPECT_EQ(ret[0], "12");
    }
    {
        const char *pattern = "^([0-9]+)([Ww])$";
        std::vector<std::string> ret;
        EXPECT_EQ(0, RegexUtil::match("12", pattern, 1, ret));
        EXPECT_TRUE(ret.empty());

        EXPECT_EQ(1, RegexUtil::match("12w", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12");
        EXPECT_EQ(ret[1], "w");
    }
    {
        const char *pattern = "^([0-9A-Za-z]+)([Ll])$";
        std::vector<std::string> ret;
        EXPECT_EQ(0, RegexUtil::match("12", pattern, 2, ret));
        EXPECT_TRUE(ret.empty());

        EXPECT_EQ(1, RegexUtil::match("12abcL", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12abc");
        EXPECT_EQ(ret[1], "L");
    }
    {
        const char *pattern = "^([0-9A-Za-z]+)\\#([0-9]+)$";
        std::vector<std::string> ret;
        EXPECT_EQ(0, RegexUtil::match("12", pattern, 2, ret));
        EXPECT_TRUE(ret.empty());

        EXPECT_EQ(1, RegexUtil::match("12abc#23", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12abc");
        EXPECT_EQ(ret[1], "23");
    }
    {
        const char *pattern = "^([0-9A-Za-z]*\\*?)\\/([0-9A-Za-z]+)$";
        std::vector<std::string> ret;

        EXPECT_EQ(1, RegexUtil::match("12abZ*/2B", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12abZ*");
        EXPECT_EQ(ret[1], "2B");
    }
}