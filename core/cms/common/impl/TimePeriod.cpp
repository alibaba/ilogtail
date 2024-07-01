/*
 * Cron表达式，有两个冲突的文档:
 * 1) https://www.netiq.com/documentation/cloud-manager-2-5/ncm-reference/data/bexyssf.html
 * 2) https://www.manpagez.com/man/5/crontab/
 * 两者的冲突之处在于day of week的表达, 7L: 有认为最后一个周日(0 or 7是周日)，有认为最后一个周6(1 - 7 mean Sun to Sat)
 * 参考本文以前的实现，我们使用1)， 7 - Saturday
 * 7 - Saturday只适用于Cron表达式。至于实现中，则使用std::tm的规范，例外是: MonthlyCalendar(1 ~ 7 means Mon ~ Sun)
 */
#include "common/TimePeriod.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/RegexUtil.h"

#include <unordered_map>
#include <algorithm> // std::reverse
#include <map>

#include <cstring> // strtok_s

#ifdef min
#   undef min
#endif

#if !defined(WIN32)
// posix
#   define strtok_s strtok_r
#endif

using namespace std;
using namespace common;

static constexpr int START_YEAR = 1970;

static int searchMap(const std::unordered_map<std::string, int> &data, const std::string &key) {
    auto it = data.find(StringUtils::ToUpper(key));
    return it == data.end() ? -1 : it->second;
}

static const auto &weekDayMap = *new std::unordered_map<std::string, int>{
        {"SUN", 0},
        {"MON", 1},
        {"TUE", 2},
        {"WED", 3},
        {"THU", 4},
        {"FRI", 5},
        {"SAT", 6},
};

static int weekDayToNum(const std::string &day) {
    return searchMap(weekDayMap, day);
}

// Cron表达式,月份， 0 ~ 11
static const auto &monthMap = *new std::unordered_map<std::string, int>{
        {"JAN", 0},
        {"FEB", 1},
        {"MAR", 2},
        {"APR", 3},
        {"MAY", 4},
        {"JUN", 5},
        {"JUL", 6},
        {"AUG", 7},
        {"SEP", 8},
        {"OCT", 9},
        {"NOV", 10},
        {"DEC", 11},
};

static int monthToNum(const std::string &mon) {
    return searchMap(monthMap, mon);
}

/// ///////////////////////////////////////////////////////////////////////////////////////////////
///
TimeSlice::TimeSlice() {
    m_sec.init(60);
    m_min.init(60);
    m_hour.init(24);
    m_dMonth.init(32); // skip 0, 1 ~ 31
    m_month.init(12);  // 0 ~ 11
    m_dWeek.init(7);   // Sun: 0, Mon: 1, ..., Sat: 6
    m_year.init(130);  // 1970 ~ 2100
}

TimeSlice::~TimeSlice() = default;

static std::string BitsYearToString(const Bits &bits) {
    const char *sep = "";
    const int size = bits.getSlot();
    std::stringstream ss;
    int beginYear = START_YEAR, lastYear = START_YEAR; // [ )
    for (int i = 0; i < size; i++) {
        if (bits.test(i)) {
            if (beginYear >= lastYear) {
                beginYear = START_YEAR + i;
            }
            lastYear = START_YEAR + i + 1;
        } else {
            if (lastYear - beginYear > 2) {
                ss << sep << beginYear << "-" << (lastYear - 1);
                sep = ",";
            } else {
                for (int year = beginYear; year < lastYear; year++) {
                    ss << sep << year;
                    sep = ",";
                }
            }
            beginYear = lastYear = START_YEAR;
        }
    }
    return ss.str();
}

std::string TimeSlice::toString(const std::initializer_list<int> &itemTypes) const {
    std::map<int, std::function<void(std::stringstream &)>> maps{
            {CRON_SEC,      [&](std::stringstream &ss) { ss << "seconds: " << m_sec.toString(); }},
            {CRON_MIN,      [&](std::stringstream &ss) { ss << "minutes: " << m_min.toString(); }},
            {CRON_HOUR,     [&](std::stringstream &ss) { ss << "hours  : " << m_hour.toString(); }},
            {CRON_MONTHDAY, [&](std::stringstream &ss) { ss << "mday   : " << m_dMonth.toString(); }},
            {CRON_MONTH,    [&](std::stringstream &ss) { ss << "month  : " << m_month.toString(); }},
            {CRON_WEEKDAY,  [&](std::stringstream &ss) { ss << "wday   : " << m_dWeek.toString(); }},
            {CRON_YEAR,     [&](std::stringstream &ss) { ss << "year   : " << BitsYearToString(m_year); }},
    };

    std::stringstream ss;
    const char *sep = "";
    for (const auto &itemType: itemTypes) {
        auto it = maps.find(itemType);
        if (it != maps.end()) {
            ss << sep;
            sep = "\n";
            it->second(ss);
        }
    }
    return ss.str();
}

bool TimeSlice::in(const std::tm &timeh) const {
    MonthlyCalendar mc;
    if (!m_dMonthOpt.empty() || !m_dWeekOpt.empty()) {
        mc.doBuild(timeh);
    }

    auto testOpt = [](const std::list<TimeOpt> &opts, const MonthlyCalendar &mc, const std::tm &t) {
        return std::any_of(opts.begin(), opts.end(), [&](const TimeOpt &opt) { return opt(mc, t); });
    };

    // 成功一定得走到最后，但失败则可尽快触发
    return m_hour.test(timeh.tm_hour)
           && m_min.test(timeh.tm_min)
           && m_sec.test(timeh.tm_sec)
           && (m_dWeek.test(timeh.tm_wday) || testOpt(m_dWeekOpt, mc, timeh))
           && (m_dMonth.test(timeh.tm_mday) || testOpt(m_dMonthOpt, mc, timeh))
           && m_month.test(timeh.tm_mon)
           && m_year.test(timeh.tm_year - 70);
}

/// ///////////////////////////////////////////////////////////////////////////////////////////////
///
Bits::Bits() = default;
Bits::~Bits() = default;

void Bits::init(int slots) {
    // m_slots = slots;
    if (slots > 0) {
        bits.resize(slots); // static_cast<size_t>(slots) * 8);
    }
}

bool Bits::set(int n) {
    // if (n > m_slots || n < 0) {
    //     return false;
    // }
    // int bit = n % 8 + 1;
    // unsigned char *c = m_bits + (size_t) ceil((n + 1.0) / 8) - 1;
    // unsigned char v = 0x01 << (8 - bit);
    // unsigned char v2 = *c;
    // *c = v2 | v;
    // return true;
    bool ok = (0 <= n && n < (int) bits.size());
    if (ok) {
        bits.set(n, true);
    }
    return ok;
}

bool Bits::setAll() {
    if (!bits.empty()) {
        bits.set(0, bits.size(), true);
    }
    return true;
    // for (int i = 0; i <= m_slots; i++) {
    //     if (!set(i)) {
    //         return false;
    //     }
    // }
    // return true;
}

bool Bits::setByStep(int begin, int step) {
    bool ok = 0 <= begin && size_t(begin) < bits.size() && step > 0;
    if (ok) {
        const size_t size = bits.size();
        for (auto pos = static_cast<size_t>(begin); pos < size; pos += step) {
            bits.set(pos, true);
        }
    }
    return ok;
    // if (begin < 0 || begin > m_slots || step <= 0) {
    //     return false;
    // }
    //
    // for (int i = begin; i < m_slots; i += step) {
    //     if (!set(i)) {
    //         return false;
    //     }
    // }
    // return true;
}

int Bits::getSlot() const {
    return static_cast<int>(bits.size());
}

bool Bits::test(int n) const {
    return 0 <= n && n < static_cast<int>(bits.size()) && bits[n];
    // if (n > m_slots || n < 0) {
    //     return false;
    // }
    //
    // int bit = n % 8 + 1;
    // const unsigned char *c = m_bits + (size_t) ceil((n + 1.0) / 8) - 1;
    // unsigned char v = 0x01 << (8 - bit);
    // return (v & *c) > 0;
}

//for testing
std::string Bits::toString() const {
    std::stringstream ss;

    const size_t cap = bits.size();
    for (size_t i = 0; i < cap;) {
        const size_t end = std::min(cap, i + 4);
        for (; i < end; i++) {
            ss << (bits.test(i) ? 1 : 0);
        }
        if (i < cap) {
            ss << ((i % 8) == 0 ? " " : ",");
        }
    }

    return ss.str();
}

/// ///////////////////////////////////////////////////////////////////////////////////////////////
///

MonthlyCalendar::MonthlyCalendar() {
    dayNum = 0;
    weekNum = 0;

    for (auto &row: weekDay) {
        for (auto &cell: row) {
            cell = -1;
        }
    }

    for (auto &i: monthDay) {
        i = -1;
    }
}

MonthlyCalendar::~MonthlyCalendar() = default;

static const int8_t daysInMonths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Cron表达式，周几， 1~7 Since Monday
struct tagWeekDay {
    int wDay = 0;  // 1 ~ 7, MON ~ Sunday

    // wday: day of week, 0~6 since SUN
    explicit tagWeekDay(const std::tm &timeExp) {
        *this = timeExp;
    }

    tagWeekDay &operator=(const std::tm &timeExp) {
        this->wDay = (timeExp.tm_wday ? timeExp.tm_wday : 7);
        return *this;
    }

    tagWeekDay &operator++() {
        if (++wDay > 7) {
            wDay = 1;
        }
        return *this;
    }

    tagWeekDay &operator--() {
        if (--wDay < 1) {
            wDay = 7;
        }
        return *this;
    }
};

void MonthlyCalendar::doBuild(const std::tm &timeExp) {
    year = static_cast<int16_t>(timeExp.tm_year + 1900);
    month = static_cast<int8_t>(timeExp.tm_mon + 1);

    dayNum = daysInMonths[timeExp.tm_mon];
    if (timeExp.tm_mon == 1 && isLeapYear(year)) {
        dayNum++; // 闰年2月29天
    }

    tagWeekDay x(timeExp);
    for (int i = timeExp.tm_mday; i > 0; --i, --x) {
        monthDay[i] = static_cast<int8_t>(x.wDay);
    }
    x = timeExp;
    for (int i = timeExp.tm_mday; i <= dayNum; ++i, ++x) {
        monthDay[i] = static_cast<int8_t>(x.wDay);
    }

    weekNum = 0;
    for (int8_t i = 1; i <= dayNum; i++) {
        weekDay[weekNum][monthDay[i]] = i;
        if (monthDay[i] == 7 && i != dayNum) {
            weekNum++;
        }
    }
}

// L最后
// timeExp是否一个月的倒数第N天(N从1起)
bool MonthlyCalendar::testMonthL(const std::tm &timeExp, int n) const {
    return timeExp.tm_mday == dayNum - n + 1;
}

// L: 最后的周N
// timeExp是否本月最后一个周N, WeekL(ast)(N)
bool MonthlyCalendar::testWeekL(const std::tm &timeExp, int n) const {
    return monthDay[timeExp.tm_mday] == (n == 0 ? 7 : n) && timeExp.tm_mday + 7 > dayNum;

    // if (weekDay[weekNum][n] == timeExp.tm_mday) {
    //     return true;
    // }
    //
    // if (weekDay[weekNum][n] == -1 && weekDay[weekNum - 1][n] == timeExp.tm_mday) {
    //     return true;
    // }
    //
    // return false;
}

// Cron: "#" 字符表示每月的第几个周几，
// 例如在周字段上设置"6#3"表示在每月的第三个周六.
// 注意如果指定"6#5",正好第五周没有周六，则不会触发该配置(用在母亲节和父亲节再合适不过了)
// n从1起
bool MonthlyCalendar::testWeekWD(const std::tm &timeh, int n, int wDay) const {
    wDay = (wDay ? wDay : 7);
    n--;
    if (0 <= n && n <= weekNum && 1 <= wDay && wDay <= 7) {
        n += weekDay[0][wDay] > 0 ? 0 : 1;
        return weekDay[n][wDay] == timeh.tm_mday;
    }
    // weekDay[week][wDay] == timeh.tm_mday;
    return false;
}

// W: 有效工作日
// 离mDay最近的工作日
bool MonthlyCalendar::testMonthW(const std::tm &timeh, int mDay) const {
    int day = 0;
    if (1 <= mDay && mDay <= dayNum) {
        if (1 <= monthDay[mDay] && monthDay[mDay] <= 5) {
            // 工作日
            day = mDay;
        } else if (monthDay[mDay] == 6) {
            // 周六
            day = (mDay > 1 ? mDay - 1 : mDay + 2);
            // if (mDay > 1) {
            //     day = mDay - 1;
            // } else {
            //     day = mDay + 2;
            // }
        } else if (monthDay[mDay] == 0 || monthDay[mDay] == 7) {
            // 周日
            day = (mDay < dayNum ? mDay + 1 : mDay - 2);
            // if (mDay <= dayNum - 1) {
            //     day = mDay + 1;
            // } else {
            //     day = mDay - 2;
            // }
        }
    }

    return day == timeh.tm_mday;
}

// 是否月最后一个工作日， 例如最后一个周三、最后一个周五
bool MonthlyCalendar::testMonthLW(const std::tm &timeh) const {
    return 1 <= timeh.tm_mday && timeh.tm_mday <= dayNum // 合法性
           && dayNum < timeh.tm_mday + 7  // 最后7天
           && 1 <= monthDay[timeh.tm_mday] && monthDay[timeh.tm_mday] <= 5; // 工作日
    // const auto &d = weekDay[weekNum];
    // if ((d[2] <= timeh.tm_mday && timeh.tm_mday <= d[6])
    //     || (d[2] <= timeh.tm_mday && d[6] == -1)
    //     || (d[2] == -1 && timeh.tm_mday <= d[6])) {
    //     return true;
    // }
    //
    // return false;
}

std::ostream &common::operator<<(std::ostream &os, const MonthlyCalendar &mc) {
    os << "   Mon Tue Wed Thu Fri Sat Sun <- " << fmt::format("{}-{:02d}", mc.year, mc.month) << std::endl;

    for (int j = 0; j <= mc.weekNum; j++) {
        os << (j + 1) << ":";
        for (int i = 1; i <= 7; i++) {
            if (mc.weekDay[j][i] > 0) {
                os << fmt::format(" {:3d}", mc.weekDay[j][i]);
            } else if (j == 0) {
                // 第一周的前置空白
                os << "    ";
            }
        }
        if (j != mc.weekNum) {
            os << std::endl;
        }
    }

    return os;
}

static bool isEmpty(const char *s) {
    return s == nullptr || *s == '\0';
}

/// ///////////////////////////////////////////////////////////////////////////////////////////////
///
TimePeriod::TimePeriod() = default;

TimePeriod::~TimePeriod() = default;

static int convertKey(int item_type, const std::string &key) {
    int num = -1;
    if (item_type == CRON_MONTH) {
        num = monthToNum(key);
    } else if (item_type == CRON_WEEKDAY) {
        num = weekDayToNum(key);
    }
    return num;
}

static int convertNum(int item_type, const std::string &s) {
    int num = convert<int>(s);
    if (item_type == CRON_YEAR) {
        num -= 1970;
    } else if (item_type == CRON_MONTH) {
        num--; // 转为0-11
    } else if (item_type == CRON_WEEKDAY) {
        // cron中周的表示为1-Sunday, 2-Tuesday, ..., 7-Saturday
        // 转换成为: 0-Sunday, 1-Monday, 2-Tuesday, ..., 6-Saturday
        num = (1 <= num && num <= 7 ? num - 1 : -1);
    }
    return num;
}

bool TimePeriod::parseItem(char *item, Bits &bits, int item_type, TimeSlice &ts) {
    if (isEmpty(item)) {
        return false;
    }

    auto checkWeekDay = [](int weekDay) {
        return 0 <= weekDay && weekDay <= 6;
    };

    char *last = nullptr;
    const char *sep = ",";
    char *piece = strtok_s(item, sep, &last);
    while (piece != nullptr) {
        // char *ret[10];
        std::vector<std::string> ret;
        if (piece[0] == '*' && piece[1] == '\0') {
            // all
            return bits.setAll();
        } else if (piece[0] == '?' && piece[1] == '\0') {
            return (item_type == CRON_MONTHDAY || item_type == CRON_WEEKDAY) && bits.setAll();
            // if (item_type != CRON_MONTHDAY && item_type != CRON_WEEKDAY) {
            //     return false;
            // }
            //
            // // all
            // return bits.setAll();
        } else if (RegexUtil::match(piece, "^([0-9]+)$", 1, ret)) {
            // the exact number
            // from 1900
            int num = convertNum(item_type, ret[0]);//(int) apr_atoi64(ret[0]);
            // if (item_type == CRON_YEAR && num >= 1970) {
            //     num -= 1970;
            // } else if (item_type == CRON_MONTH) {
            //     num--; // 转为0-11
            // }

            if (!bits.set(num)) {
                return false;
            }
        } else if (StringUtils::EqualIgnoreCase(piece, "L")) {
            // last day of month or last day of week
            if (item_type == CRON_WEEKDAY) {
                if (!bits.set(0)) { // 周日
                    return false;
                }
            } else if (item_type == CRON_MONTHDAY) {
                // TimeOpt to;
                // to.optType = TIME_OPT_MONTH_L;
                // to.arg1 = 1;
                ts.m_dMonthOpt.emplace_back([=](const MonthlyCalendar &mc, const std::tm &t) {
                    return mc.testMonthL(t, 1);
                });
            } else {
                return false;
            }
        } else if (RegexUtil::match(piece, "^([0-9]+)([Ww])$", 2, ret)) {
            if (item_type != CRON_MONTHDAY) {
                return false;
            }
            // 距离num日最近的工作日
            int num = convertNum(item_type, ret[0]); // (int) apr_atoi64(ret[0]);
            if (num <= 0 || num > 31) {
                return false;
            }

            // TimeOpt to;
            // to.optType = TIME_OPT_MONTH_W;
            // to.arg1 = num;
            ts.m_dMonthOpt.emplace_back([=](const MonthlyCalendar &mc, const std::tm &t) {
                return mc.testMonthW(t, num);
            });

        } else if (StringUtils::EqualIgnoreCase(piece, "LW") || StringUtils::EqualIgnoreCase(piece, "WL")) {
            // last working day of month
            if (item_type != CRON_MONTHDAY) {
                return false;
            }

            // TimeOpt to;
            // to.optType = TIME_OPT_MONTH_LW;
            ts.m_dMonthOpt.emplace_back([=](const MonthlyCalendar &mc, const std::tm &t) { return mc.testMonthLW(t); });
        } else if (RegexUtil::match(piece, "^([0-9A-Za-z]+)([Ll])$", 2, ret)) {
            // last n day of month or last n day of week day
            if (item_type == CRON_WEEKDAY) {
                int num;
                if (RegexUtil::match(ret[0], "^[0-9]+$")) {
                    num = convertNum(item_type, ret[0]); // (int) apr_atoi64(ret[0]);
                } else {
                    num = convertKey(item_type, ret[0]);
                }
                if (!checkWeekDay(num)) {
                    return false;
                }

                // TimeOpt to;
                // to.optType = TIME_OPT_WEEK_L;
                // to.arg1 = num;
                ts.m_dWeekOpt.emplace_back([=](const MonthlyCalendar &mc, const std::tm &t) {
                    return mc.testWeekL(t, num);
                });
            } else if (item_type == CRON_MONTHDAY && RegexUtil::match(ret[0], "^[0-9]+$")) {
                // to.optType = TIME_OPT_MONTH_L;
                int num = convertNum(item_type, ret[0]); // (int) apr_atoi64(ret[0]);
                if (num <= 0 || num > 31) {
                    return false;
                }
                // TimeOpt to;
                // to.arg1 = num;
                ts.m_dMonthOpt.emplace_back([=](const MonthlyCalendar &mc, const std::tm &t) {
                    return mc.testMonthL(t, num);
                });
            } else {
                return false;
            }
        } else if (RegexUtil::match(piece, "^([0-9A-Za-z]+)\\#([0-9]+)$", 2, ret)) {
            if (item_type != CRON_WEEKDAY) {
                return false;
            }

            // The value of 6#3 in the day-of-week field means the third Friday of the month (day 6 = Friday and #3 = the 3rd one in the month).
            //
            // Other Examples:
            // 2#1 specifies the first Monday of the month and
            // 4#5 specifies the fifth Wednesday of the month. However,
            // if you specify #5 and there are fewer than 5 of the given day-of-week in the month, no firing occurs that month.
            int dayOfWeek;
            if (RegexUtil::match(ret[0], "^[0-9]+$")) {
                dayOfWeek = convertNum(item_type, ret[0]); // (int) apr_atoi64(ret[0]);
            } else {
                // monthly day or weekly day
                dayOfWeek = convertKey(item_type, ret[0]);
            }

            if (!checkWeekDay(dayOfWeek)) {
                return false;
            }

            int num2 = convertNum(CRON_UNKNOWN, ret[1]); // (int) apr_atoi64(ret[1]);
            if (num2 <= 0 || 5 < num2) {
                return false;
            }

            // TimeOpt to;
            // to.optType = TIME_OPT_WEEK_WD;
            // to.arg1 = dayOfWeek;
            // to.arg2 = num2;
            ts.m_dWeekOpt.emplace_back([=](const MonthlyCalendar &mc, const std::tm &t) {
                return mc.testWeekWD(t, num2, dayOfWeek);
            });

        } else if (RegexUtil::match(piece, "^([A-Za-z]+)$", 1, ret)) {
            int num = convertKey(item_type, ret[0]);
            // // the exact number
            // if (item_type == CRON_MONTH) {
            //     num = monthToNum(ret[0]);
            // } else if (item_type == CRON_WEEKDAY) {
            //     num = weekDayToNum(ret[0]);
            // }

            if (!bits.set(num)) {
                return false;
            }
        } else if (RegexUtil::match(piece, "^([0-9A-Za-z]+)\\-([0-9A-Za-z]+)$", 2, ret)) {
            // range
            std::vector<int> range;
            range.reserve(2);
            for (const auto &str: ret) {
                if (RegexUtil::match(str, "^[0-9]+$")) {
                    range.push_back(convertNum(item_type, str)); // (int) apr_atoi64(ret[0]);
                    // if (item_type == CRON_YEAR && num1 >= 1970) {
                    //     num1 -= 1970;
                    // } else if (item_type == CRON_MONTH) {
                    //     num1--;
                    // }
                } else {
                    range.push_back(convertKey(item_type, str));
                    // // monthly day or weekly day
                    // if (item_type == CRON_MONTH) {
                    //     num1 = monthToNum(ret[0]);
                    // } else if (item_type == CRON_WEEKDAY) {
                    //     num1 = weekDayToNum(ret[0]);
                    // }
                }
            }
            int &num1 = range[0], num2 = range[1];
            if (num2 > num1 && num1 >= 0) {
                for (int i = num1; i <= num2; i++) {
                    if (!bits.set(i)) {
                        return false;
                    }
                }
            } else {
                return false;
            }
        } else if (RegexUtil::match(piece, "^([0-9A-Za-z]+|\\*)/(\\d+)$", 2, ret)) {
            int num1;
            // begin and step
            if (ret[0] == "*" || ret[0] == "0") {
                num1 = 0;
            } else if (RegexUtil::match(ret[0], "^[0-9]+$")) {
                num1 = convertNum(item_type, ret[0]);
            } else {
                num1 = convertKey(item_type, ret[0]);
            }

            int span = convertNum(CRON_UNKNOWN, ret[1]);

            if (!bits.setByStep(num1, span)) {
                return false;
            }
        } else {
            // syntax error
            return false;
        }

        piece = strtok_s(nullptr, sep, &last);
    }

    return true;
}

bool TimePeriod::parse(const char *timeStr) {
    if (isEmpty(timeStr)) {
        return false;
    }

    m_timeSlice.clear();

    // apr_pool_t *p;
    // apr_pool_create(&p, NULL);

    // char *str = strdup(timeStr);
    // defer(free(str));
    std::string localCache(timeStr);
    char *str = const_cast<char *>(localCache.c_str());

    const char *sep = "\r\n[]";
    char *nextItem = nullptr;
    char *item = strtok_s(str, sep, &nextItem);

    while (item != nullptr) {
        auto slice = std::make_shared<TimeSlice>();

        std::string tmp(item);
        char *newItem = const_cast<char *>(tmp.c_str());

        constexpr const char *sep2 = " \t";
        char *nextItem2 = nullptr;

#define FIND_ITEM() strtok_s(newItem, sep2, &nextItem2)
#define PARSE_ITEM(item2, bits, itemType, itemTypeName)                          \
        if (!parseItem(item2, bits, itemType, *slice)) {                         \
            LogError("parse {} error: '{}' of '{}'", itemTypeName, item2, item); \
            return false;                                                        \
        }
#define FIND_AND_PARSE_ITEM(bits, itemType) {            \
            char *item2 = FIND_ITEM();                   \
            PARSE_ITEM(item2, bits, itemType, #itemType) \
        }

        // parse sec
        FIND_AND_PARSE_ITEM(slice->m_sec, CRON_SEC)
        newItem = nullptr;
        // if (!parseItem(item2, slice->m_sec, CRON_SEC, *slice)) {
        //     LogError("parse sec error:{}({})", item, item2);
        //     // apr_pool_destroy(p);
        //     return false;
        // }

        // parse min
        FIND_AND_PARSE_ITEM(slice->m_min, CRON_MIN)
        // item2 = strtok_s(nullptr, sep2, &nextItem2);
        // if (!parseItem(item2, slice->m_min, CRON_MIN, *slice)) {
        //     LogError("parse min error:{}({})", item, item2);
        //     // apr_pool_destroy(p);
        //     return false;
        // }

        // parse hour
        FIND_AND_PARSE_ITEM(slice->m_hour, CRON_HOUR)
        // item2 = strtok_s(nullptr, sep2, &nextItem2);
        // if (!parseItem(item2, slice->m_hour, CRON_HOUR, *slice)) {
        //     LogError("parse hour error:{}({})", item, item2);
        //     // apr_pool_destroy(p);
        //     return false;
        // }

        // parse day of month
        FIND_AND_PARSE_ITEM(slice->m_dMonth, CRON_MONTHDAY)
        // item2 = strtok_s(nullptr, sep2, &nextItem2);
        // if (!parseItem(item2, slice->m_dMonth, CRON_MONTHDAY, *slice)) {
        //     LogError("parse day of month error:{}({})", item, item2);
        //     // apr_pool_destroy(p);
        //     return false;
        // }

        // parse month
        FIND_AND_PARSE_ITEM(slice->m_month, CRON_MONTH)
        // item2 = strtok_s(nullptr, sep2, &nextItem2);
        // if (!parseItem(item2, slice->m_month, CRON_MONTH, *slice)) {
        //     LogError("parse month error:{}({})", item, item2);
        //     // apr_pool_destroy(p);
        //     return false;
        // }

        // parse day of week
        FIND_AND_PARSE_ITEM(slice->m_dWeek, CRON_WEEKDAY)
        // item2 = strtok_s(nullptr, sep2, &nextItem2);
        // if (!parseItem(item2, slice->m_dWeek, CRON_WEEKDAY, *slice)) {
        //     LogError("parse day of week error:{}({})", item, item2);
        //     // apr_pool_destroy(p);
        //     return false;
        // }

        // parse year
        // item2 = strtok_s(nullptr, sep2, &nextItem2);
        {
            char *item2 = FIND_ITEM();
            if (isEmpty(item2)) {
                slice->m_year.setAll();
            } else {
                PARSE_ITEM(item2, slice->m_year, CRON_YEAR, "CRON_YEAR")
            }
        }
#undef FIND_AND_PARSE_ITEM
#undef PARSE_ITEM
#undef FIND_ITEM
        m_timeSlice.push_back(slice);
        item = strtok_s(nullptr, sep, &nextItem);
    }

    // apr_pool_destroy(p);

    return !m_timeSlice.empty();
}

bool TimePeriod::in(const std::tm &expt) const {
    return std::any_of(m_timeSlice.begin(), m_timeSlice.end(), [&](const std::shared_ptr<TimeSlice> &ts) {
        return ts->in(expt);
    });
}

bool TimePeriod::in(const std::chrono::system_clock::time_point &now) const {
    bool ok = !this->empty();
    if (ok) {
        std::tm expt = fmt::localtime(std::chrono::system_clock::to_time_t(now));
        ok = in(expt);
    }
    return ok;
}
