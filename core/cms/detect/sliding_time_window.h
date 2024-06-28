#ifndef _SLIDING_TIME_WINDOW_H_
#define _SLIDING_TIME_WINDOW_H_
#include <cstdint>
#include <vector>
#include "common/Chrono.h"

namespace common
{

template<class T>
class SlidingTimeWindow
{
public:
    explicit SlidingTimeWindow(const std::chrono::microseconds &);
    ~SlidingTimeWindow();
    void Update(const T &value);
    void RemoveTimeoutValue();
    T GetMaxValue();
    T GetMinValue();
    double GetAvgValue();
private:
    const std::chrono::microseconds mWindowTime; // 微秒级时间戳
    std::vector<std::chrono::system_clock::time_point> mTimes;
    std::vector<T> mValues;
};

template<class T>
SlidingTimeWindow<T>::SlidingTimeWindow(const std::chrono::microseconds &micros):mWindowTime(micros) {
    mValues.reserve(128);
    mTimes.reserve(128);
}

template<class T>
SlidingTimeWindow<T>::~SlidingTimeWindow()
{
    mTimes.clear();
    mValues.clear();
}
template<class T>
void SlidingTimeWindow<T>::Update(const T &value)
{
    mTimes.push_back(std::chrono::Now<ByMicros>());
    mValues.push_back(value);
}
template<class T>
void SlidingTimeWindow<T>::RemoveTimeoutValue()
{
    auto now = std::chrono::Now<ByMicros>();
    auto itTime = mTimes.begin();
    auto itValue = mValues.begin();
    for (; itTime != mTimes.end();)
    {
        if (*itTime + mWindowTime < now)
        {
            itTime = mTimes.erase(itTime);
            itValue = mValues.erase(itValue);
        }
        else
        {
            //默认时间越来越大
            break;
        }
    }
}
template<class T>
T SlidingTimeWindow<T>::GetMaxValue()
{
    if (mValues.empty())
    {
        return 0;
    }
    T max = mValues[0];
    for (size_t i = 1; i < mValues.size(); i++)
    {
        if (mValues[i] > max)
        {
            max = mValues[i];
        }
    }
    return max;
}
template<class T>
T SlidingTimeWindow<T>::GetMinValue()
{
    if (mValues.empty())
    {
        return 0;
    }
    T min = mValues[0];
    for (size_t i = 1; i < mValues.size(); i++)
    {
        if (mValues[i] < min)
        {
            min = mValues[i];
        }
    }
    return min;
}
template<class T>
double SlidingTimeWindow<T>::GetAvgValue()
{
    if (mValues.empty())
    {
        return 0.0;
    }
    double total = 0.0;
    for (T n: mValues)
    {
        total += n;
    }
    return total / mValues.size();
}
}
#endif
