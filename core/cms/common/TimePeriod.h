/**
 * @file TimePeriod.h
 *
 * @brief parser quartz
 * @brief see time is in quartz
 *
 * @version 0.1 2009-06-12
 * @since 0.1
 */
#ifndef ARGUS_AGENT_TIME_PERIOD_H
#define ARGUS_AGENT_TIME_PERIOD_H

#include <iostream>
#include <vector>
#include <chrono>
#include <list>
#include <functional>

#include <boost/dynamic_bitset.hpp>

namespace common {

#include "test_support"
class Bits {
public:
    Bits();
    ~Bits();

    void init(int slots);
    bool set(int n);
    bool setAll();
    bool setByStep(int begin, int step);
    int getSlot() const;
    bool test(int n) const;

    //for testing
    std::string toString() const;

private:
    boost::dynamic_bitset<> bits;
};

#define CRON_UNKNOWN (-1)
#define CRON_SEC      0
#define CRON_MIN      1
#define CRON_HOUR     2
#define CRON_MONTHDAY 3
#define CRON_MONTH    4
#define CRON_WEEKDAY  5
#define CRON_YEAR     6


class MonthlyCalendar {
    friend std::ostream &operator<<(std::ostream &os, const MonthlyCalendar &);
public:
    MonthlyCalendar();
    ~MonthlyCalendar();

    void doBuild(const std::tm &timeExp);

    bool testMonthL(const std::tm &, int n) const;
    bool testWeekL(const std::tm &, int n) const;
    bool testWeekWD(const std::tm &, int arg1, int arg2) const;
    bool testMonthW(const std::tm &, int n) const;
    bool testMonthLW(const std::tm &) const;

private:
    int16_t year = 0;
    int8_t month = 0;
    // total day num of this month
    int8_t dayNum = 0;
    // total week num of this month
    int8_t weekNum = 0;
    // each weekly day's month day
    int8_t weekDay[6][8]{};
    // each monthly day's weekly day
    int8_t monthDay[32]{};
};

std::ostream &operator<<(std::ostream &os, const MonthlyCalendar &);

class TimeSlice {
public:
    typedef std::function<bool(const MonthlyCalendar &, const std::tm &)> TimeOpt;
    TimeSlice();
    ~TimeSlice();
    bool in(const std::tm &) const;
    std::string toString(const std::initializer_list<int> &itemTypes) const;

    Bits m_sec;
    Bits m_min;
    Bits m_hour;
    Bits m_dMonth;
    std::list<TimeOpt> m_dMonthOpt;
    Bits m_month;
    Bits m_dWeek;
    std::list<TimeOpt> m_dWeekOpt;
    Bits m_year;
};

class TimePeriod {
public:
    TimePeriod();
    ~TimePeriod();

    bool parse(const char *timeStr);
    bool in(const std::tm &expt) const;
    bool in(const std::chrono::system_clock::time_point &now) const;
    bool empty() const {
        return m_timeSlice.empty();
    }

    void reset() {
        m_timeSlice.clear();
    }

private:
    static bool parseItem(char *item, Bits &bits, int itemType, TimeSlice &ts);
    std::vector<std::shared_ptr<TimeSlice>> m_timeSlice;
};
#include "test_support"

} // end of namespace common

#endif
