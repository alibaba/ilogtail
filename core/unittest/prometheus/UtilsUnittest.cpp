
#include "models/StringView.h"
#include "prometheus/Utils.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

bool IsDoubleEqual(double a, double b) {
    return fabs(a - b) < 0.000001;
}

class PromUtilsUnittest : public testing::Test {
public:
    void TestDurationToSecond();
};

void PromUtilsUnittest::TestDurationToSecond() {
    string rawData = "30s";
    APSARA_TEST_EQUAL(30ULL, DurationToSecond(rawData));
}

UNIT_TEST_CASE(PromUtilsUnittest, TestDurationToSecond);

} // namespace logtail

UNIT_TEST_MAIN
