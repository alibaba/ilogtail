// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <string>

#include <json/json.h>

#include "common/JsonUtil.h"
#include "pipeline/limiter/ConcurrencyLimiter.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class ConcurrencyLimiterUnittest : public testing::Test {
public:
    void TestLimiter() const;
};

void ConcurrencyLimiterUnittest::TestLimiter() const {
    shared_ptr<ConcurrencyLimiter> sConcurrencyLimiter = make_shared<ConcurrencyLimiter>(80, 1, 20);
    // comcurrency = 10, count = 0
    APSARA_TEST_EQUAL(true, sConcurrencyLimiter->IsValidToPop());
    sConcurrencyLimiter->PostPop();
    APSARA_TEST_EQUAL(1U, sConcurrencyLimiter->GetSendingCount());
    sConcurrencyLimiter->OnFail(time(NULL));
    sConcurrencyLimiter->OnSendDone();
    APSARA_TEST_EQUAL(10U, sConcurrencyLimiter->GetLimit());
    APSARA_TEST_EQUAL(0U, sConcurrencyLimiter->GetSendingCount());
    APSARA_TEST_EQUAL(10U, sConcurrencyLimiter->GetLimit());
    APSARA_TEST_EQUAL(90U, sConcurrencyLimiter->GetInterval());

    // count = 10, comcurrency = 10
    APSARA_TEST_EQUAL(true, sConcurrencyLimiter->IsValidToPop());
    int num = 10;
    for (int i = 0; i < num; i++) {
        APSARA_TEST_EQUAL(true, sConcurrencyLimiter->IsValidToPop());
        sConcurrencyLimiter->PostPop();
    }
    APSARA_TEST_EQUAL(10U, sConcurrencyLimiter->GetSendingCount());
    for (int i = 0; i < num; i++) {
        sConcurrencyLimiter->OnSuccess();
        sConcurrencyLimiter->OnSendDone();
    }
    APSARA_TEST_EQUAL(0U, sConcurrencyLimiter->GetSendingCount());
    APSARA_TEST_EQUAL(20U, sConcurrencyLimiter->GetLimit());
    APSARA_TEST_EQUAL(30U, sConcurrencyLimiter->GetInterval());


    // interval = 30 * 1.5 * 1.5 * 1.5 = 67 * 1.5 * 1.5 = 100 * 1.5 = 150 
    num = 4;
    for (int i = 0; i < num; i++) {
        APSARA_TEST_EQUAL(true, sConcurrencyLimiter->IsValidToPop());
        sConcurrencyLimiter->PostPop();
    }
    APSARA_TEST_EQUAL(4U, sConcurrencyLimiter->GetSendingCount());
    for (int i = 0; i < num; i++) {
        sConcurrencyLimiter->OnFail(time(NULL));
        sConcurrencyLimiter->OnSendDone();
    }
    APSARA_TEST_EQUAL(0U, sConcurrencyLimiter->GetSendingCount());
    APSARA_TEST_EQUAL(1U, sConcurrencyLimiter->GetLimit());
    APSARA_TEST_EQUAL(150U, sConcurrencyLimiter->GetInterval());

    // interval = 150 * 1.5
    num = 3;
    for (int i = 0; i < num; i++) {
        if (i >= 1) {
            APSARA_TEST_EQUAL(false, sConcurrencyLimiter->IsValidToPop());
        } else {
            APSARA_TEST_EQUAL(true, sConcurrencyLimiter->IsValidToPop());
            sConcurrencyLimiter->PostPop();
        }
    }
    APSARA_TEST_EQUAL(1U, sConcurrencyLimiter->GetSendingCount());
    sConcurrencyLimiter->OnFail(time(NULL));
    sConcurrencyLimiter->OnSendDone();
    APSARA_TEST_EQUAL(0U, sConcurrencyLimiter->GetSendingCount());
    APSARA_TEST_EQUAL(1U, sConcurrencyLimiter->GetLimit());
    APSARA_TEST_EQUAL(225U, sConcurrencyLimiter->GetInterval());


    num = 10;

}


UNIT_TEST_CASE(ConcurrencyLimiterUnittest, TestLimiter)

} // namespace logtail

UNIT_TEST_MAIN
