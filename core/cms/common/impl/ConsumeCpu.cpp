#ifdef ENABLE_COVERAGE

#include <chrono>

#include "common/ConsumeCpu.h"

using namespace std::chrono;
static int64_t base = 500 * 1000 * 1000;

// 运行控制在50ms左右
void consumeCpu(int64_t millis) {
    steady_clock::time_point now = steady_clock::now();
    steady_clock::time_point expired = now + milliseconds{millis};
    do {
        volatile int64_t total = 0;
        for (int64_t j = 0; j < base; j++) {
            total += j * j - j * 10;
        }
    } while (steady_clock::now() < expired);

    if (millis == 50) {
        auto actual = duration_cast<milliseconds>(steady_clock::now() - now);
        if (actual > milliseconds{millis}) {
            base /= static_cast<double>(actual.count()) / double(millis);
        }
    }
}

#endif
