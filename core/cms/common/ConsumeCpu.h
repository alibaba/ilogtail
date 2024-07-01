#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif
#ifndef ARGUS_COMMON_CONSUME_CPU_H
#define ARGUS_COMMON_CONSUME_CPU_H

#ifdef ENABLE_COVERAGE
#include <cstdint>
void consumeCpu(int64_t millis = 50);
#endif

#endif // !ARGUS_COMMON_CONSUME_CPU_H
