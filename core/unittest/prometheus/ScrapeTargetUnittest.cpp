
#include <string>

#include "Labels.h"
#include "ScrapeTarget.h"
#include "StringTools.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {
class ScrapeTargetUnittest : public testing::Test {
public:
    void TestInit();
};

void ScrapeTargetUnittest::TestInit() {
    Labels labels;
    labels.Push(Label{"__address__", "127.0.0.1:8080"});
    labels.Push(Label{"ip", "127.0.0.1"});
    labels.Push(Label{"port", "8080"});
    ScrapeTarget target;
    target = ScrapeTarget(labels);

    APSARA_TEST_EQUAL("127.0.0.1", target.mHost);
    APSARA_TEST_EQUAL(8080, target.mPort);
    APSARA_TEST_EQUAL("127.0.0.1:8080" + ToString(labels.Hash()), target.GetHash());

    // default port when port is not error
    labels = Labels();
    labels.Push(Label{"__address__", "127.0.0.1:3333333"});
    target = ScrapeTarget(labels);

    APSARA_TEST_EQUAL("127.0.0.1", target.mHost);
    APSARA_TEST_EQUAL(9100, target.mPort);
}

UNIT_TEST_CASE(ScrapeTargetUnittest, TestInit);

} // namespace logtail

UNIT_TEST_MAIN