//
// Created by 韩呈杰 on 2023/5/4.
//
#include <gtest/gtest.h>
#include "common/TimePeriod.h"

#include "common/Logger.h"
#include "common/ChronoLiteral.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"

using namespace std;
using namespace common;

#if defined(__linux__) || defined(__APPLE__)

#include <regex.h>

bool reg_match(const char *str, const char *pattern) {
    regex_t reg;
    regmatch_t pm[1];
    const size_t nmatch = 1;

    int res = regcomp(&reg, pattern, REG_EXTENDED);

    if (res != 0) {
        regfree(&reg);
        return false;
    }

    res = regexec(&reg, str, nmatch, pm, 0);

    bool r = (res != REG_NOMATCH);
    // if (res == REG_NOMATCH) { r = false; }
    // else { r = true; }

    regfree(&reg);

    return r;
}

#define MAX_MATCH_NUM   10

static short reg_match_fetch(const char *str, const char *pattern,
                             short mcount, std::vector<std::string> &ret) {
    auto nmatch = (size_t) (mcount + 1);
    int res = 0, i = 0, n = 0;
    short r = 0;
    regmatch_t pm[MAX_MATCH_NUM];
    regex_t reg;

    res = regcomp(&reg, pattern, REG_EXTENDED);
    if (res != 0) {
        char buf[256];
        regerror(res, &reg, buf, sizeof(buf));
        LogError("reg compile error: {}", buf);
        regfree(&reg);
        return 0;
    }

    res = regexec(&reg, str, nmatch, pm, 0);
    if (res == REG_NOMATCH) {
        r = 0;
    } else {
        for (i = 1; i <= mcount; i++) {
            if (pm[i].rm_so == -1) {
                break;
            }
            n = pm[i].rm_eo - pm[i].rm_so;
            const char *s = str + pm[i].rm_so;
            ret.emplace_back(s, s + n); //apr_pstrndup(mp, str + pm[i].rm_so, n);
        }
        r = 1;
    }
    regfree(&reg);
    /* return if matched */
    return r;
}

TEST(CommonTimePeriodTest, RegMatch) {
    EXPECT_TRUE(reg_match("1", "^[0-9]+$"));
    EXPECT_TRUE(reg_match("12", "^[0-9]+$"));

    EXPECT_FALSE(reg_match(" 1", "^[0-9]+$"));
    EXPECT_FALSE(reg_match(" 12", "^[0-9]+$"));
}

TEST(CommonTimePeriodTest, RegMatchFetch) {
    {
        std::vector<std::string> ret;
        EXPECT_EQ(1, reg_match_fetch("1", "^([0-9]+)$", 1, ret));
        EXPECT_EQ(size_t(1), ret.size());
        EXPECT_EQ(ret[0], "1");
    }
    {
        std::vector<std::string> ret;
        EXPECT_EQ(1, reg_match_fetch("12", "^([0-9]+)$", 1, ret));
        EXPECT_EQ(size_t(1), ret.size());
        EXPECT_EQ(ret[0], "12");
    }
    {
        const char *pattern = "^([0-9]+)([Ww])$";
        std::vector<std::string> ret;
        EXPECT_EQ(0, reg_match_fetch("12", pattern, 1, ret));
        EXPECT_TRUE(ret.empty());

        EXPECT_EQ(1, reg_match_fetch("12w", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12");
        EXPECT_EQ(ret[1], "w");
    }
    {
        const char *pattern = "^([0-9A-Za-z]+)([Ll])$";
        std::vector<std::string> ret;
        EXPECT_EQ(0, reg_match_fetch("12", pattern, 2, ret));
        EXPECT_TRUE(ret.empty());

        EXPECT_EQ(1, reg_match_fetch("12abcL", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12abc");
        EXPECT_EQ(ret[1], "L");
    }
    {
        const char *pattern = "^([0-9A-Za-z]+)\\#([0-9]+)$";
        std::vector<std::string> ret;
        EXPECT_EQ(0, reg_match_fetch("12", pattern, 2, ret));
        EXPECT_TRUE(ret.empty());

        EXPECT_EQ(1, reg_match_fetch("12abc#23", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12abc");
        EXPECT_EQ(ret[1], "23");
    }
    {
        const char *pattern = "^([0-9A-Za-z]*\\*?)\\/([0-9A-Za-z]+)$";
        std::vector<std::string> ret;

        EXPECT_EQ(1, reg_match_fetch("12abZ*/2B", pattern, 2, ret));
        EXPECT_EQ(size_t(2), ret.size());
        EXPECT_EQ(ret[0], "12abZ*");
        EXPECT_EQ(ret[1], "2B");
    }
}

#endif

TEST(CommonTimeSliceTest, toString) {
    std::initializer_list<int> lst{
            CRON_SEC,
            CRON_MIN,
            CRON_HOUR,
            CRON_MONTHDAY,
            CRON_MONTH,
            CRON_WEEKDAY,
            CRON_YEAR,
    };
    TimeSlice ts;
    EXPECT_FALSE(ts.toString(lst).empty());
}

TEST(CommonTimePeriodTest, Bits) {
    Bits bits;
    bits.init(2);
    std::cout << bits.toString() << std::endl;
    EXPECT_EQ(2, bits.getSlot());
    EXPECT_FALSE(bits.test(1));
    bits.set(1);
    std::cout << bits.toString() << std::endl;
    EXPECT_TRUE(bits.test(1));
}

TEST(CommonTimePeriodTest, parse01) {
    const char *cron = "[* * 21-23 ? * *][* * 0-8 ? * *][* 0-15 9 ? * *]";
    std::shared_ptr<TimePeriod> p_shared{new TimePeriod()};
    EXPECT_TRUE(p_shared->parse(cron));
    std::cout << "cron: " << cron << std::endl;
    for (const auto &it: p_shared->m_timeSlice) {
        std::cout << it->toString({CRON_MIN, CRON_HOUR, CRON_MONTHDAY}) << std::endl;
    }
    auto zero = std::chrono::FromSeconds(1544630400);   // 2018-12-13 00:00:00.000000

    auto t1 = zero + std::chrono::hours{3};
    EXPECT_TRUE(p_shared->in(t1));
    auto t2 = zero + std::chrono::hours{10};
    EXPECT_FALSE(p_shared->in(t2));
    auto t3 = zero + std::chrono::hours{9} + std::chrono::minutes{2};
    EXPECT_TRUE(p_shared->in(t3));
}

TEST(CommonTimePeriodTest, parse02Week) {
    struct {
        int64_t seconds;
        std::vector<const char *> weeks;
    } cases[] = {
            {1544630400 - 4 * 24 * 60 * 60, {"sun", "1"}},
            {1544630400 - 3 * 24 * 60 * 60, {"mon", "2"}},
            {1544630400 - 2 * 24 * 60 * 60, {"TUE", "3"}},
            {1544630400 - 1 * 24 * 60 * 60, {"WeD", "4"}},
            {1544630400 + 0 * 24 * 60 * 60, {"Thu", "5"}}, // 2018-12-13 00:00:00.000000 周四
            {1544630400 + 1 * 24 * 60 * 60, {"Fri", "6"}},
            {1544630400 + 2 * 24 * 60 * 60, {"sat", "7"}},
    };
    for (const auto &tc: cases) {
        for (const char *week: tc.weeks) {
            std::string cronExpr = fmt::format("* * * * * {}", week);
            TimePeriod cron;
            EXPECT_TRUE(cron.parse(cronExpr.c_str()));
            std::cout << "cron: " << cronExpr << std::endl;
            for (const auto &it: cron.m_timeSlice) {
                std::cout << it->toString({CRON_WEEKDAY}) << std::endl;
            }
            EXPECT_EQ(cron.m_timeSlice.size(), size_t(1));
            auto zero = std::chrono::FromSeconds(tc.seconds);   // 2018-12-13 00:00:00.000000 周四
            EXPECT_TRUE(cron.in(zero));
            EXPECT_FALSE(cron.in(zero + 24_h));
            EXPECT_FALSE(cron.in(zero - 1_h));
        }
    }
}

TEST(CommonTimePeriodTest, parse03Month) {
    struct {
        int64_t seconds;
        std::vector<const char *> months;
    } cases[] = {
            {1831305600, {"jan", "1"}},  // 2028-01-13 00:00:00
            {1844438400, {"jun", "6"}},  // 2028-06-13 00:00:00
            {1544630400, {"dec", "12"}}, // 2018-12-13 00:00:00.000000 周四
    };
    for (const auto &tc: cases) {
        for (const char *month: tc.months) {
            std::string cronExpr = fmt::format("* * * * {} *", month);
            TimePeriod cron;
            EXPECT_TRUE(cron.parse(cronExpr.c_str()));
            std::cout << "cron: " << cronExpr << std::endl;
            for (const auto &it: cron.m_timeSlice) {
                std::cout << it->toString({CRON_MONTH}) << std::endl;
            }
            EXPECT_EQ(cron.m_timeSlice.size(), size_t(1));
            auto zero = std::chrono::FromSeconds(tc.seconds);   // 2018-12-13 00:00:00.000000 周四
            EXPECT_TRUE(cron.in(zero));
            EXPECT_FALSE(cron.in(zero + 30 * 24_h));
            EXPECT_FALSE(cron.in(zero - 30 * 24_h));
        }
    }
}

TEST(CommonTimePeriodTest, parse04W) {
    struct {
        int mday;
        int64_t okTs;
        int64_t failTs;
    } cases[] = {
            {15, 1544716800/*2018-12-14 周五*/, 1544976000/*2018-12-17 周一*/},
            {1,  1535904000/*2018-09-03 周一*/, 1535644800/*2018-08-31 周五*/},
            {1,  1535904000/*2018-09-03 周一*/, 1535990400/*2018-09-04 周二*/},
            {3,  1535904000/*2018-09-03 周一*/, 1535990400/*2018-09-04 周二*/},
            {7,  1536249600/*2018-09-07 周五*/, 1535990400/*2018-09-04 周二*/},
            {8,  1536249600/*2018-09-07 周五*/, 1535990400/*2018-09-04 周二*/},
            {9,  1536508800/*2018-09-10 周一*/, 1535990400/*2018-09-04 周二*/},
            {15, 1536854400/*2018-09-14 周五*/, 1537113600/*2018-09-17 周一*/},
            {16, 1537113600/*2018-09-17 周一*/, 1536854400/*2018-09-14 周五*/},

            {30, 1682611200/*2023-04-28 周五*/, 1682784000/*2023-04-30 周日*/},
    };
    for (const auto &tc: cases) {
        std::string cronExpr = fmt::format("* * * {}W * *", tc.mday);
        TimePeriod cron;
        EXPECT_TRUE(cron.parse(cronExpr.c_str()));
        std::cout << "cron: " << cronExpr << std::endl;
        // for (const auto &it: cron.m_timeSlice) {
        //     std::cout << it->toString({CRON_MONTHDAY}) << std::endl;
        // }
        EXPECT_EQ(cron.m_timeSlice.size(), size_t(1));

        EXPECT_TRUE(cron.in(std::chrono::FromSeconds(tc.okTs)));
        EXPECT_FALSE(cron.in(std::chrono::FromSeconds(tc.failTs)));
    }
}

TEST(CommonTimePeriodTest, parse05_L) {
    {
        // invalid
        std::string cronExpr = "L * * * * *";
        TimePeriod cron;
        EXPECT_FALSE(cron.parse(cronExpr.c_str()));
    }
    {
        // month day
        std::string cronExpr = "* * * L * ?";
        TimePeriod cron;
        EXPECT_TRUE(cron.parse(cronExpr.c_str()));
        std::cout << "cron: " << cronExpr << std::endl;
        EXPECT_EQ(cron.m_timeSlice.size(), size_t(1));
        EXPECT_TRUE(cron.in(std::chrono::FromSeconds(1535644800))); // 2018-08-31 Friday
    }
    {
        // week day
        std::string cronExpr = "* * * * * L";
        TimePeriod cron;
        EXPECT_TRUE(cron.parse(cronExpr.c_str()));
        std::cout << "cron: " << cronExpr << std::endl;
        EXPECT_EQ(cron.m_timeSlice.size(), size_t(1));
        EXPECT_FALSE(cron.in(std::chrono::FromSeconds(1535212800 - 24 * 60 * 60)));
        EXPECT_TRUE(cron.in(std::chrono::FromSeconds(1535212800))); // 2018-08-26 Sunday
        EXPECT_FALSE(cron.in(std::chrono::FromSeconds(1535212800 + 24 * 60 * 60)));
    }
}

TEST(CommonTimePeriodTest, parse06_Error) {
    const char *cronExpresses[] = {
            nullptr,
            "",

            "? * * * * *",
            "* ? * * * *",
            "* * ? * * *",
            "* * * * ? *",
            "* * * * * * ?",

            "* 2W * * * *",  // Not In MonthDay
            "* LW * * * *",  // Not In MonthDay
            "* * *  0W * *", // MonthDay overflow
            "* * * 50W * *", // MonthDay overflow

            "* * * SUN * *", // Not In WeekDay
            "* * * * * 0l",  // WeekDay overflow
            "* * * * * 8l",  // WeekDay overflow

            "* * * 1#2 * *", // Not In WeekDay
            "* * * * * 0#3", // pound sign overflow
            "* * * * * 7#6", // pound sign overflow

            "70 * * * * *",
            "* 70 * * * *",
            "* * 70 * * *",
            "* * * 35 * *",
            "* * * 35L * *",
            "* * * 30-35 * *",
            "* * * * 13 *",  // 0 ~ 11
            "* * * * * 0",   // 1 ~ 7
            "* * * * * 8",   // 1 ~ 7
            "@ * * * * *",   // 1 ~ 7
    };
    for (size_t i = 0; i < sizeof(cronExpresses) / sizeof(cronExpresses[0]); i++) {
        const char *cronExpr = cronExpresses[i];
        LogInfo("[{:2d}] {}", i, (cronExpr? cronExpr: "<nil>"));
        TimePeriod cron;
        EXPECT_FALSE(cron.parse(cronExpr));
    }
}

TEST(CommonTimePeriodTest, parse07_LW) {
    const char *cronExpresses[] = {
            "* * * lw * *",
            "* * * LW * *",
            "* * * lW * *",
            "* * * Lw * *",
            "* * * wl * *",
            "* * * WL * *",
            "* * * Wl * *",
            "* * * wL * *",
    };
    MonthlyCalendar mc;
    mc.doBuild(fmt::localtime(1735056000));
    std::cout << mc;
    for (size_t i = 0; i < sizeof(cronExpresses) / sizeof(cronExpresses[0]); i++) {
        const char *cronExpr = cronExpresses[i];
        LogInfo("[{:2d}] {}", i, cronExpr);
        TimePeriod cron;
        EXPECT_TRUE(cron.parse(cronExpr));
        EXPECT_FALSE(cron.empty());

        struct {
            int64_t ts;
            bool match;
        } testCases[] = {
                {1735056000 - 2 * 24 * 60 * 60, false},  // 2024-12-23 周一 (倒数第二个周一)
                {1735056000 - 1 * 24 * 60 * 60, false},  // 2024-12-24 周二 (倒数第二个周二)
                {1735056000 - 0 * 24 * 60 * 60, true},   // 2024-12-25 周三
                {1735056000 + 1 * 24 * 60 * 60, true},   // 2024-12-26 周四
                {1735056000 + 2 * 24 * 60 * 60, true},   // 2024-12-27 周五
                {1735056000 + 3 * 24 * 60 * 60, false},  // 2024-12-28 周六 (非工作日)
                {1735056000 + 4 * 24 * 60 * 60, false},  // 2024-12-29 周日 (非工作日)
                {1735056000 + 5 * 24 * 60 * 60, true},   // 2024-12-30 周一
                {1735056000 + 6 * 24 * 60 * 60, true},   // 2024-12-31 周二
                {1735056000 + 7 * 24 * 60 * 60, false},  // 2025-01-01 周三
        };
        for (const auto &tc: testCases) {
            EXPECT_EQ(tc.match, cron.in(std::chrono::FromSeconds(tc.ts)));
        }

        cron.reset();
        EXPECT_TRUE(cron.empty());
    }
}

TEST(CommonTimePeriodTest, parse08_LastSatOfMonth) {
    const char *cronExpresses[] = {
            "* * * * * 7L",    // 最后一个周六
            "* * * * * SATL",  // 最后一个周六
            "* * * * * 7l",    // 最后一个周六
            "* * * * * SAtl",  // 最后一个周六
    };
    for (const char *cronExpr: cronExpresses) {
        std::cout << cronExpr << std::endl;

        TimePeriod cron;
        EXPECT_TRUE(cron.parse(cronExpr));

        struct {
            int64_t ts;
            bool match;
        } testCases[] = {
                {1735315200 - 7 * 24 * 60 * 60, false}, // 2024-12-21 周六
                {1735315200 - 1 * 24 * 60 * 60, false}, // 2024-12-27 周五
                {1735315200 + 0 * 24 * 60 * 60, true},  // 2024-12-28 周六
                {1735315200 + 1 * 24 * 60 * 60, false}, // 2024-12-29 周日
        };
        for (const auto &tc: testCases) {
            EXPECT_EQ(tc.match, cron.in(std::chrono::FromSeconds(tc.ts)));
        }
    }
}

TEST(CommonTimePeriodTest, parse08_LastNDayOfMonth) {
    const char *cronExpresses[] = {
            "* * * {}L * *",
            "* * * {}l * *",
    };
    for (const char *pattern: cronExpresses) {
        for (int n = 1; n <= 7; n++) {
            std::string cronExpr = fmt::format(pattern, n);
            std::cout << cronExpr << std::endl;

            TimePeriod cron;
            EXPECT_TRUE(cron.parse(cronExpr.c_str()));

            struct {
                int64_t ts;
                bool match;
            } testCases[] = {
                    {1703952000 - (n - 1 - 1) * 24 * 60 * 60, false}, // 2023-12-30
                    {1703952000 - (n - 1 + 0) * 24 * 60 * 60, true},  // 2023-12-31
                    {1703952000 - (n - 1 + 1) * 24 * 60 * 60, false}, // 2024-01-01

                    {1709136000 - (n - 1 - 1) * 24 * 60 * 60, false}, // 2023-02-28
                    {1709136000 - (n - 1 + 0) * 24 * 60 * 60, true},  // 2023-02-29
                    {1709136000 - (n - 1 + 1) * 24 * 60 * 60, false}, // 2023-03-01
            };
            for (const auto &tc: testCases) {
                EXPECT_EQ(tc.match, cron.in(std::chrono::FromSeconds(tc.ts)));
            }
        }
    }
}

TEST(CommonTimePeriodTest, parse09_Year) {
    const char *szCronExpr = "* * * * * * 2019,2020,2022-2024";
    std::cout << szCronExpr << std::endl;

    TimePeriod cron;
    EXPECT_TRUE(cron.parse(szCronExpr));
    ASSERT_EQ(size_t(1), cron.m_timeSlice.size());
    std::string str = cron.m_timeSlice[0]->toString({CRON_YEAR});
    std::cout << str << std::endl;
    EXPECT_EQ(str, "year   : 2019,2020,2022-2024");

    struct {
        int64_t ts;
        bool match;
    } testCases[] = {
            {1640966399999999, false}, // 2021-12-31 23:59:59.999999
            {1640966400000000, true},  // 2022-01-01 00:00:00
            {1704038399999999, true},  // 2023-12-31 23:59:59.999999
            {1735660799999999, true},  // 2024-12-31 23:59:59.999999
            {1735660800000000, false}, // 2025-01-01 00:00:00.000000
    };
    for (const auto &tc: testCases) {
        EXPECT_EQ(tc.match, cron.in(std::chrono::FromMicros(tc.ts)));
    }
}

TEST(CommonTimePeriodTest, parse10_PoundSign) {
    const char *szCronExpresses[] = {
            "* * * * * 6#2",    // 第二个周五
            "* * * * * Fri#2",  // 第二个周五
    };
    for (const char *szCronExpr: szCronExpresses) {
        std::cout << szCronExpr << std::endl;

        TimePeriod cron;
        EXPECT_TRUE(cron.parse(szCronExpr));
        ASSERT_EQ(size_t(1), cron.m_timeSlice.size());

        struct {
            int64_t ts;
            bool match;
        } testCases[] = {
                {1672934400, false}, // 2023-01-06 00:00:00
                {1673539200, true},  // 2023-01-13 00:00:00
                {1674144000, false}, // 2023-01-20 00:00:00
                {1678377600, true},  // 2023-03-10 00:00:00
                {1681401600, true},  // 2023-04-14 00:00:00
                {1694102400, true},  // 2023-09-08 00:00:00
        };
        for (const auto &tc: testCases) {
            EXPECT_EQ(tc.match, cron.in(std::chrono::FromSeconds(tc.ts)));
        }
    }
}

TEST(CommonTimePeriodTest, parse11_PoundSign_Sunday) {
    const char *szCronExpresses[] = {
            "* * * * * 1#5",    // 第五个周日
            "* * * * * Sun#5",
    };
    for (const char *szCronExpr: szCronExpresses) {
        std::cout << szCronExpr << std::endl;

        TimePeriod cron;
        EXPECT_TRUE(cron.parse(szCronExpr));
        ASSERT_EQ(size_t(1), cron.m_timeSlice.size());

        struct {
            int64_t ts;
            bool match;
        } testCases[] = {
                {1674921600 - 7 * 24 * 60 * 60, false},  // 2023-01-22 00:00:00
                {1674921600,                    true},   // 2023-01-29 00:00:00
        };
        for (const auto &tc: testCases) {
            EXPECT_EQ(tc.match, cron.in(std::chrono::FromSeconds(tc.ts)));
        }
    }
}

TEST(CommonTimePeriodTest, parse12_WeekDayRange) {
    const char *szCronExpr = "* * * * * Mon-6"; // 周一到周五
    std::cout << szCronExpr << std::endl;

    TimePeriod cron;
    EXPECT_TRUE(cron.parse(szCronExpr));
    ASSERT_EQ(size_t(1), cron.m_timeSlice.size());

    struct {
        int64_t ts;
        bool match;
    } testCases[] = {
            {1674748800,                    true},   // 2023-01-27 00:00:00 周五
            {1674748800 + 1 * 24 * 60 * 60, false},  // 2023-01-28 00:00:00 周六
            {1674748800 + 2 * 24 * 60 * 60, false},  // 2023-01-29 00:00:00 周日
            {1674748800 + 3 * 24 * 60 * 60, true},   // 2023-01-30 00:00:00 周一
            {1674748800 + 4 * 24 * 60 * 60, true},   // 2023-01-31 00:00:00 周二
            {1674748800 + 5 * 24 * 60 * 60, true},   // 2023-02-01 00:00:00 周三
            {1674748800 + 6 * 24 * 60 * 60, true},   // 2023-02-02 00:00:00 周四
            {1674748800 + 7 * 24 * 60 * 60, true},   // 2023-02-03 00:00:00 周五
            {1674748800 + 8 * 24 * 60 * 60, false},  // 2023-02-04 00:00:00 周六
    };
    for (const auto &tc: testCases) {
        EXPECT_EQ(tc.match, cron.in(std::chrono::FromSeconds(tc.ts)));
    }
}

TEST(CommonTimePeriodTest, parse13_Slash) {
    const char *szCronExpressions[] = {
            "* * * */10 * *",
            "* * * 0/10 * *",
    };
    for (const char *szCronExpr: szCronExpressions) {
        std::cout << szCronExpr << std::endl;

        TimePeriod cron;
        EXPECT_TRUE(cron.parse(szCronExpr));
        ASSERT_EQ(size_t(1), cron.m_timeSlice.size());

        constexpr int64_t Day = 24 * 60 * 60;
        const int64_t start = 1672502400 - Day; // 2023-01-01
        for (size_t i = 1; i <= 31; i++) {
            bool expect = (i % 10) == 0;
            EXPECT_EQ(expect, cron.in(std::chrono::FromSeconds(start + i * Day)));
        }
    }
}

TEST(CommonTimePeriodTest, parse13_Slash_WeekDay) {
    const char *szCronExpressions[] = {
            "* * * * * Sun/2",
            "* * * * * 1/2",
    };
    for (const char *szCronExpr: szCronExpressions) {
        std::cout << szCronExpr << std::endl;

        TimePeriod cron;
        EXPECT_TRUE(cron.parse(szCronExpr));
        ASSERT_EQ(size_t(1), cron.m_timeSlice.size());
        std::cout << cron.m_timeSlice[0]->toString({CRON_WEEKDAY}) << std::endl;
        EXPECT_EQ(cron.m_timeSlice[0]->toString({CRON_WEEKDAY}), "wday   : 1010,101");

        constexpr int64_t Day = 24 * 60 * 60;
        const int64_t start = 1672502400; // 2023-01-01, Sunday
        for (int64_t i = 0; i <= 31; i++) {
            auto seconds = start + i * Day;
            auto now = std::chrono::FromSeconds(seconds);
            auto tm = fmt::localtime(seconds);
            bool expect = (tm.tm_wday % 2) == 0;
            const char *chineseDigit[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
            LogInfo("{:L%F} {}", now, chineseDigit[tm.tm_wday]);  // 2023-01-01 Sunday
            EXPECT_EQ(expect, cron.in(now));
        }
    }
}

TEST(CommonTimePeriodTest, ParseItem) {
    TimePeriod cron;

    {
        TimeSlice ts;
        EXPECT_FALSE(cron.parseItem(nullptr, ts.m_year, CRON_YEAR, ts));
    }
    {
        TimeSlice ts;
        Bits bits;
        char item[] = "L";
        EXPECT_FALSE(cron.parseItem(item, bits, CRON_WEEKDAY, ts));
        char item2[] = "L";
        EXPECT_FALSE(cron.parseItem(item2, bits, CRON_WEEKDAY, ts));
    }
}

TEST(CommonMonthlyCalendarTest, testWeekWD) {
    constexpr const int64_t _7day = 7 * 24 * 60 * 60;
    constexpr const int64_t ts = 1612108800; // 2021-02-01 该月正好『满』4周(02-01周一，共28天)，没有第5周

    MonthlyCalendar mc;
    mc.doBuild(fmt::localtime(ts));
    std::cout << mc << std::endl;
    EXPECT_EQ(3, (int)mc.weekNum); // 只跨了4个周，一般是5个，少数跨6个。

    const char *szExpect = R"(
   Mon Tue Wed Thu Fri Sat Sun <- 2021-02
1:   1   2   3   4   5   6   7
2:   8   9  10  11  12  13  14
3:  15  16  17  18  19  20  21
4:  22  23  24  25  26  27  28
)";
    std::stringstream ss;
    ss << mc;
    EXPECT_EQ(TrimRightSpace(ss.str()), TrimRightSpace(szExpect + 1));

    EXPECT_FALSE(mc.testWeekWD(fmt::localtime(ts + 0 * _7day), 1, 9)); // wDay溢出
    EXPECT_TRUE (mc.testWeekWD(fmt::localtime(ts + 0 * _7day), 1, 1)); // 第一个周一
    EXPECT_TRUE (mc.testWeekWD(fmt::localtime(ts + 1 * _7day), 2, 1)); // 第二个周一
    EXPECT_TRUE (mc.testWeekWD(fmt::localtime(ts + 2 * _7day), 3, 1));
    EXPECT_TRUE (mc.testWeekWD(fmt::localtime(ts + 3 * _7day), 4, 1));
    EXPECT_FALSE(mc.testWeekWD(fmt::localtime(ts + 4 * _7day), 5, 1)); // overflow of weekNum
}

TEST(CommonMonthlyCalendarTest, doBuild01) {
    MonthlyCalendar calendar;
    std::tm tm = fmt::localtime(time_t(1700559408)); // 2023-11-21 17:36:48 CST
    calendar.doBuild(tm);
    std::cout << calendar << std::endl;
    EXPECT_EQ(calendar.dayNum, 30);
    EXPECT_EQ(calendar.monthDay[1], 3); // 2023-11-01为周三

    const char *szExpect = R"(
   Mon Tue Wed Thu Fri Sat Sun <- 2023-11
1:           1   2   3   4   5
2:   6   7   8   9  10  11  12
3:  13  14  15  16  17  18  19
4:  20  21  22  23  24  25  26
5:  27  28  29  30)";
    std::stringstream ss;
    ss << calendar;
    EXPECT_EQ(ss.str(), TrimRightSpace(szExpect + 1));
}

TEST(CommonMonthlyCalendarTest, doBuild02) {
    // 2023-01-01 08:00:00 +0800 周日
    std::tm tm = fmt::localtime(std::time_t(1672531200));
    EXPECT_EQ(tm.tm_year, 2023 - 1900);
    EXPECT_EQ(tm.tm_mon, 0);
    EXPECT_EQ(tm.tm_mday, 1);
    EXPECT_EQ(tm.tm_wday, 0);

    MonthlyCalendar mc;
    mc.doBuild(tm);
    EXPECT_EQ(mc.dayNum, 31);
    EXPECT_EQ(mc.weekNum, 5); // 横跨6周
    EXPECT_EQ(mc.monthDay[1], 7);
    std::cout << mc << std::endl;

    const char *szExpect = R"(
   Mon Tue Wed Thu Fri Sat Sun <- 2023-01
1:                           1
2:   2   3   4   5   6   7   8
3:   9  10  11  12  13  14  15
4:  16  17  18  19  20  21  22
5:  23  24  25  26  27  28  29
6:  30  31)";
    std::stringstream ss;
    ss << mc;
    EXPECT_EQ(ss.str(), TrimRightSpace(szExpect + 1));
}

TEST(CommonMonthlyCalendarTest, doBuild03) {
    // 2023-04-11 08:00:00 +0800 周二
    // 该月最后一天正好是周日, weekNum没有多计数
    std::tm tm = fmt::localtime(std::time_t(1681171200));
    EXPECT_EQ(tm.tm_year, 2023 - 1900);
    EXPECT_EQ(tm.tm_mon, 3);
    EXPECT_EQ(tm.tm_mday, 11);
    EXPECT_EQ(tm.tm_wday, 2);

    MonthlyCalendar mc;
    mc.doBuild(tm);
    EXPECT_EQ(mc.dayNum, 30);
    EXPECT_EQ(mc.weekNum, 4); // 横跨5周
    EXPECT_EQ(mc.monthDay[1], 6);
    std::cout << mc << std::endl;

    const char *szExpect = R"(
   Mon Tue Wed Thu Fri Sat Sun <- 2023-04
1:                       1   2
2:   3   4   5   6   7   8   9
3:  10  11  12  13  14  15  16
4:  17  18  19  20  21  22  23
5:  24  25  26  27  28  29  30)";
    std::stringstream ss;
    ss << mc;
    EXPECT_EQ(ss.str(), TrimRightSpace(szExpect + 1));
}

TEST(CommonMonthlyCalendarTest, doBuild04) {
    // 2024-02-02 08:00:00 +0800 周四
    std::tm tm = fmt::localtime(std::time_t(1706745600));
    EXPECT_EQ(tm.tm_year, 2024 - 1900);
    EXPECT_EQ(tm.tm_mon, 1);
    EXPECT_EQ(tm.tm_mday, 1);
    EXPECT_EQ(tm.tm_wday, 4);

    MonthlyCalendar mc;
    mc.doBuild(tm);
    EXPECT_EQ(mc.dayNum, 29); // 闰年，29天
    EXPECT_EQ(mc.weekNum, 4); // 横跨4周
    EXPECT_EQ(mc.monthDay[1], 4);
    std::cout << mc << std::endl;

    const char *szExpect = R"(
   Mon Tue Wed Thu Fri Sat Sun <- 2024-02
1:               1   2   3   4
2:   5   6   7   8   9  10  11
3:  12  13  14  15  16  17  18
4:  19  20  21  22  23  24  25
5:  26  27  28  29)";
    std::stringstream ss;
    ss << mc;
    EXPECT_EQ(ss.str(), TrimRightSpace(szExpect + 1));
}
