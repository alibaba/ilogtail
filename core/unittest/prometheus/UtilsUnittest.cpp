
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

void PromUtilsUnittest::TestStringViewToDouble() {
    StringView normal("123.456");
    StringView normal1("-123.456");
    StringView normal2("123.456e-3");
    StringView normal3(" 0X1.BC70A3D70A3D7P+6");
    StringView error1("123.456.789");
    StringView error2("123.456 789");
    StringView error3("1.18973e+4932");
    double res = 0;

    APSARA_TEST_TRUE(StringToDouble(normal, res));
    APSARA_TEST_TRUE(IsDoubleEqual(123.456, res));

    APSARA_TEST_TRUE(StringToDouble(normal1, res));
    APSARA_TEST_TRUE(IsDoubleEqual(-123.456, res));

    APSARA_TEST_TRUE(StringToDouble(normal2, res));
    APSARA_TEST_TRUE(IsDoubleEqual(123.456e-3, res));

    APSARA_TEST_TRUE(StringToDouble(normal3, res));
    APSARA_TEST_TRUE(IsDoubleEqual(111.110000, res));

    APSARA_TEST_FALSE(StringToDouble(error1, res));
    APSARA_TEST_FALSE(StringToDouble(error2, res));
    APSARA_TEST_FALSE(StringToDouble(error3, res));
}


UNIT_TEST_CASE(PromUtilsUnittest, TestStringViewToDouble)


} // namespace logtail

UNIT_TEST_MAIN
