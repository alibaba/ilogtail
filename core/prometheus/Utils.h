#pragma once


#include <string>
namespace logtail {

std::string URLEncode(const std::string& value);

std::string SecondToDuration(uint64_t duration);
uint64_t DurationToSecond(const std::string& duration);

uint64_t GetRandSleepMilliSec(const std::string& key, uint64_t intervalSeconds, uint64_t currentMilliSeconds);
} // namespace logtail
