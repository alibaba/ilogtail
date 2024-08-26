
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
    void TestStringViewToDouble();
};


UNIT_TEST_CASE(PromUtilsUnittest, TestStringViewToDouble)


} // namespace logtail

UNIT_TEST_MAIN
