// Copyright 2022 iLogtail Authors
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

#include "unittest/Unittest.h"
#include "unittest/UnittestHelper.h"
#include "observer/interface/helper.h"
#include "observer/network/protocols/utils.h"
#include "network/protocols/mysql/parser.h"


namespace logtail {

struct TestReq {
    int num;
    uint64_t TimeNano;
    int ID;
    TestReq() { std::cout << "construct req" << std::endl; }
    ~TestReq() { std::cout << "deconstruct req" << std::endl; }
    std::string ToString() {
        std::stringstream ss;
        ss << "TestReq num: " << num << " timenano: " << TimeNano << " id: " << ID;
        return ss.str();
    }
    void Clear() {
        num = 0;
        TimeNano = 0;
        ID = 0;
    }
};

struct TestResp {
    int num;
    uint64_t TimeNano;
    int ID;
    TestResp() { std::cout << "construct resp" << std::endl; }
    ~TestResp() { std::cout << "deconstruct resp" << std::endl; }
    std::string ToString() {
        std::stringstream ss;
        ss << "TestResp num: " << num << " timenano: " << TimeNano << " id: " << ID;
        return ss.str();
    }
    void Clear() {
        num = 0;
        TimeNano = 0;
        ID = 0;
    }
};


typedef CommonCache<TestReq, TestResp, MySQLProtocolEventAggregator, MySQLProtocolEvent, 4> TestCache;
typedef CommonMapCache<TestReq, TestResp, int, MySQLProtocolEventAggregator, MySQLProtocolEvent, 2> TestMapCache;

struct ExpectTestCacheMeta {
    size_t headRequest = 0;
    size_t headResponse = 0;
    size_t tailRequest = -1;
    size_t tailResponse = -1;
    size_t reqSize = 0;
    size_t respSize = 0;
};


class ProtocolUtilUnittest : public ::testing::Test {
public:
    void TestReadUntil() {
        char s[] = "\x50\x00\x00\x00\x28\x00\x73\x65\x6c\x65\x63\x74\x20\x2a\x20\x66"
                   "\x72\x6f\x6d\x20\x70\x67\x5f\x61\x6d\x20\x77\x68\x65\x72\x65\x20"
                   "\x6f\x69\x64\x3d\x20\x32\x00\x00\x00";
        ProtoParser parser(&s[0], 41, true);
        APSARA_TEST_TRUE((*parser.readChar()) == 'P');
        APSARA_TEST_EQUAL(parser.readUint32(), 40);
        APSARA_TEST_TRUE(parser.readUntil('\0') == "");
        APSARA_TEST_TRUE(parser.readUntil('\0') == "select * from pg_am where oid= 2");
        APSARA_TEST_EQUAL(parser.readUint16(), 0);
    }

    void TestSlsStringPiece() {
        logtail::SlsStringPiece d1("abc", 3);
        APSARA_TEST_TRUE(d1.TrimToString() == "abc");

        logtail::SlsStringPiece d2("abc  ", 5);
        APSARA_TEST_TRUE(d2.TrimToString() == "abc");

        logtail::SlsStringPiece d3(" abc", 4);
        APSARA_TEST_TRUE(d3.TrimToString() == "abc");


        logtail::SlsStringPiece d4(" abc ", 5);
        APSARA_TEST_TRUE(d4.TrimToString() == "abc");

        logtail::SlsStringPiece d5(" \tabc  \t", 8);
        APSARA_TEST_TRUE(d5.TrimToString() == "abc");

        logtail::SlsStringPiece v1("Ram-Sdk-Hostip", 14);
        logtail::SlsStringPiece v2("Content-Length", 14);

        std::map<logtail::SlsStringPiece, int> header1;

        header1[v1] = 1;
        header1[v1] = 1;
        APSARA_TEST_TRUE(header1.size() == 1);

        std::map<logtail::SlsStringPiece, int> header2;

        header2[v1] = 1;
        header2[v2] = 1;

        APSARA_TEST_TRUE(header2.size() == 2);

        static logtail::SlsStringPiece v3("Content-Length", 14);

        APSARA_TEST_TRUE(header2.find(v3) != header2.end());

        logtail::SlsStringPiece v4("abc", 3);
        logtail::SlsStringPiece v5("ab", 2);
        logtail::SlsStringPiece v6("abcd", 4);
        logtail::SlsStringPiece v7("aBc", 3);
        logtail::SlsStringPiece v8("Ab", 2);
        logtail::SlsStringPiece v9("abcD", 4);
        logtail::SlsStringPiece v10("", 0);
        logtail::SlsStringPiece v11("abc", 3);
        logtail::SlsStringPiece v12("", 0);

        APSARA_TEST_EQUAL(v4 < v5, false);
        APSARA_TEST_EQUAL(v5 < v4, true);

        APSARA_TEST_EQUAL(v4 < v6, true);
        APSARA_TEST_EQUAL(v6 < v4, false);

        APSARA_TEST_EQUAL(v4 < v7, false);
        APSARA_TEST_EQUAL(v7 < v4, true);

        APSARA_TEST_EQUAL(v4 < v8, false);
        APSARA_TEST_EQUAL(v8 < v4, true);

        APSARA_TEST_EQUAL(v4 < v9, true);
        APSARA_TEST_EQUAL(v9 < v4, false);

        APSARA_TEST_EQUAL(v4 < v10, false);
        APSARA_TEST_EQUAL(v10 < v4, true);

        APSARA_TEST_EQUAL(v4 < v11, false);
        APSARA_TEST_EQUAL(v11 < v4, false);

        APSARA_TEST_EQUAL(v10 < v12, false);
        APSARA_TEST_EQUAL(v12 < v10, false);
    }

    void PrintCache(TestCache& cache, ExpectTestCacheMeta meta) {
        std::cout << "=============================\n"
                  << "req head: " << cache.mHeadRequestsIdx << "req tail:" << cache.mTailRequestsIdx
                  << "req size:" << cache.GetRequestsSize() << "resp head: " << cache.mHeadResponsesIdx
                  << "resp tail:" << cache.mTailResponsesIdx << "resp size:" << cache.GetResponsesSize() << std::endl;

        APSARA_TEST_EQUAL(cache.mHeadRequestsIdx, meta.headRequest);
        APSARA_TEST_EQUAL(cache.mTailRequestsIdx, meta.tailRequest);
        APSARA_TEST_EQUAL(cache.mHeadResponsesIdx, meta.headResponse);
        APSARA_TEST_EQUAL(cache.mTailResponsesIdx, meta.tailResponse);
        APSARA_TEST_EQUAL(cache.GetRequestsSize(), meta.reqSize);
        APSARA_TEST_EQUAL(cache.GetResponsesSize(), meta.respSize);
    }

    void TestCommonCacheContinueReq() {
        TestCache cache(nullptr);
        for (int i = 0; i < 4; ++i) {
            cache.GetReqByIndex(i)->num = i;
            cache.GetRespByIndex(i)->num = i;
        }
        for (int i = 0; i < 16; ++i) {
            cache.GetReqPos()->TimeNano = GetCurrentTimeInNanoSeconds();
            ExpectTestCacheMeta meta;
            meta.headRequest = i < 4 ? 0 : i - 3;
            meta.tailRequest = meta.headRequest + (i < 4 ? i : 3);
            meta.reqSize = meta.tailRequest - meta.headRequest + 1;
            cache.TryStitcherByReq();
            PrintCache(cache, meta);
        }
    }

    void TestCommonCacheContinueResp() {
        TestCache cache(nullptr);
        for (int i = 0; i < 4; ++i) {
            cache.GetReqByIndex(i)->num = i;
            cache.GetRespByIndex(i)->num = i;
        }
        for (int i = 0; i < 16; ++i) {
            cache.GetRespPos()->TimeNano = GetCurrentTimeInNanoSeconds();
            ExpectTestCacheMeta meta;
            meta.headResponse = i < 4 ? 0 : i - 3;
            meta.tailResponse = meta.headResponse + (i < 4 ? i : 3);
            meta.respSize = meta.tailResponse - meta.headResponse + 1;
            cache.TryStitcherByResp();
            PrintCache(cache, meta);
        }
    }

    void TestCommonCacheContinueOneByOneAndReqFirst() {
        TestCache cache(nullptr);
        for (int i = 0; i < 4; ++i) {
            cache.GetReqByIndex(i)->num = i;
            cache.GetRespByIndex(i)->num = i;
        }
        int count = 0;
        cache.BindConvertFunc([&](TestReq* req, TestResp* resp, MySQLProtocolEvent& e) -> bool {
            count++;
            return false;
        });
        for (int i = 0; i < 16; ++i) {
            cache.GetReqPos()->TimeNano = GetCurrentTimeInNanoSeconds();
            cache.GetRespPos()->TimeNano = GetCurrentTimeInNanoSeconds();
            ExpectTestCacheMeta meta;
            meta.headRequest = i;
            meta.tailRequest = meta.headRequest;
            meta.headResponse = i;
            meta.tailResponse = meta.headResponse;
            meta.reqSize = 1;
            meta.respSize = 1;
            PrintCache(cache, meta);
            cache.TryStitcherByResp();
            ++meta.headRequest;
            meta.tailRequest = meta.headRequest - 1;
            meta.headResponse = meta.headRequest;
            meta.tailResponse = meta.tailRequest;
            meta.reqSize = 0;
            meta.respSize = 0;
            PrintCache(cache, meta);
            APSARA_TEST_EQUAL(count, i + 1);
        }
    }

    void TestCommonCacheContinueOneByOneAndRespFirst() {
        TestCache cache(nullptr);
        for (int i = 0; i < 4; ++i) {
            cache.GetReqByIndex(i)->num = i;
            cache.GetRespByIndex(i)->num = i;
        }
        int count = 0;
        cache.BindConvertFunc([&](TestReq* req, TestResp* resp, MySQLProtocolEvent& e) -> bool {
            count++;
            return false;
        });
        for (int i = 0; i < 16; ++i) {
            auto before = GetCurrentTimeInNanoSeconds();
            cache.GetRespPos()->TimeNano = GetCurrentTimeInNanoSeconds();
            cache.GetReqPos()->TimeNano = before;
            ExpectTestCacheMeta meta;
            meta.headRequest = i;
            meta.tailRequest = meta.headRequest;
            meta.headResponse = i;
            meta.tailResponse = meta.headResponse;
            meta.reqSize = 1;
            meta.respSize = 1;
            PrintCache(cache, meta);
            cache.TryStitcherByReq();
            ++meta.headRequest;
            meta.tailRequest = meta.headRequest - 1;
            meta.headResponse = meta.headRequest;
            meta.tailResponse = meta.tailRequest;
            meta.reqSize = 0;
            meta.respSize = 0;
            PrintCache(cache, meta);
            APSARA_TEST_EQUAL(count, i + 1);
        }
    }

    void TestCommonCacheInsertOldResp() {
        TestCache cache(nullptr);
        for (int i = 0; i < 4; ++i) {
            cache.GetReqByIndex(i)->num = i;
            cache.GetRespByIndex(i)->num = i;
        }
        int count = 0;
        cache.BindConvertFunc([&](TestReq* req, TestResp* resp, MySQLProtocolEvent& e) -> bool {
            count++;
            return false;
        });
        auto time = GetCurrentTimeInNanoSeconds();
        cache.GetReqPos()->TimeNano = time;

        for (int i = 0; i < 16; ++i) {
            cache.GetRespPos()->TimeNano = time - 2;
            cache.TryStitcherByResp();
            APSARA_TEST_EQUAL(count, 0);
            ExpectTestCacheMeta meta;
            meta.headRequest = 0;
            meta.tailRequest = 0;
            meta.headResponse = i + 1;
            meta.tailResponse = i;
            meta.reqSize = 1;
            meta.respSize = 0;
            PrintCache(cache, meta);
        }
    }

    void TestCommonCacheInsertNewReq() {
        TestCache cache(nullptr);
        for (int i = 0; i < 4; ++i) {
            cache.GetReqByIndex(i)->num = i;
            cache.GetRespByIndex(i)->num = i;
        }
        int count = 0;
        cache.BindConvertFunc([&](TestReq* req, TestResp* resp, MySQLProtocolEvent& e) -> bool {
            count++;
            return false;
        });
        auto time = GetCurrentTimeInNanoSeconds();
        cache.GetRespPos()->TimeNano = time - 3;
        cache.GetRespPos()->TimeNano = time - 2;
        cache.GetReqPos()->TimeNano = time;
        cache.TryStitcherByReq();
        APSARA_TEST_EQUAL(cache.GetRequestsSize(), 1);
        APSARA_TEST_EQUAL(cache.GetResponsesSize(), 0);
        APSARA_TEST_EQUAL(count, 0);
        ExpectTestCacheMeta meta;
        meta.headRequest = 0;
        meta.tailRequest = 0;
        meta.headResponse = 2;
        meta.tailResponse = 1;
        meta.reqSize = 1;
        meta.respSize = 0;
        PrintCache(cache, meta);
    }

    void TestCommonCacheTryMatchingReq() {
        TestCache cache(nullptr);
        for (int i = 0; i < 4; ++i) {
            cache.GetReqByIndex(i)->num = i;
            cache.GetRespByIndex(i)->num = i;
        }
        int count = 0;
        cache.BindConvertFunc([&](TestReq* req, TestResp* resp, MySQLProtocolEvent& e) -> bool {
            count++;
            APSARA_TEST_EQUAL(resp->TimeNano - req->TimeNano, 2);
            return false;
        });
        auto time = GetCurrentTimeInNanoSeconds();
        cache.GetReqPos()->TimeNano = time - 3;
        cache.GetReqPos()->TimeNano = time - 2;
        cache.GetReqPos()->TimeNano = time + 1;
        cache.GetRespPos()->TimeNano = time;
        cache.TryStitcherByResp();
        APSARA_TEST_EQUAL(cache.GetRequestsSize(), 1);
        APSARA_TEST_EQUAL(cache.GetResponsesSize(), 0);
        APSARA_TEST_EQUAL(count, 1);
    }

    void TestCommonMapCacheContinueReq() {
        TestMapCache cache(nullptr);
        for (int i = 0; i < 10; ++i) {
            auto success = cache.InsertReq(i, [&](TestReq* req) {
                req->ID = i;
                req->TimeNano = i;
            });
            if (i >= 8) {
                APSARA_TEST_FALSE(success);
            } else {
                APSARA_TEST_TRUE(success);
            }
        }
        APSARA_TEST_EQUAL(cache.mReqManager.mUnUsed.size(), 0);
        bool success = cache.GarbageCollection(2000);
        APSARA_TEST_TRUE(success);
        APSARA_TEST_EQUAL(cache.mReqManager.mUnUsed.size(), 2);
    }

    void TestCommonMapCacheContinueResp() {
        TestMapCache cache(nullptr);
        for (int i = 0; i < 10; ++i) {
            auto success = cache.InsertResp(i, [&](TestResp* resp) -> void {
                resp->ID = i;
                resp->TimeNano = i;
            });
            if (i >= 8) {
                APSARA_TEST_FALSE(success);
            } else {
                APSARA_TEST_TRUE(success);
            }
        }
        APSARA_TEST_EQUAL(cache.mRespManager.mUnUsed.size(), 0);
        bool success = cache.GarbageCollection(2000);
        APSARA_TEST_TRUE(success);
        APSARA_TEST_EQUAL(cache.mRespManager.mUnUsed.size(), 2);
    }


    void TestCommonMapCacheOneByOne() {
        TestMapCache cache(nullptr);
        for (int i = 0; i < 10; ++i) {
            auto success = cache.InsertResp(i, [&](TestResp* resp) -> void {
                resp->ID = i;
                resp->TimeNano = i;
            });
            APSARA_TEST_TRUE(success);
            APSARA_TEST_EQUAL(cache.mResponses.size(), 1);
            success = cache.InsertReq(i, [&](TestReq* req) -> void {
                req->ID = i;
                req->TimeNano = i;
            });
            APSARA_TEST_TRUE(success);
            APSARA_TEST_EQUAL(cache.mResponses.size(), 0);
            APSARA_TEST_EQUAL(cache.mRequests.size(), 0);
            APSARA_TEST_EQUAL(cache.mRespManager.mUnUsed.size(), 1);
            APSARA_TEST_EQUAL(cache.mReqManager.mUnUsed.size(), 0);
        }

        TestMapCache cache2(nullptr);
        for (int i = 0; i < 10; ++i) {
            auto success = cache2.InsertReq(i, [&](TestReq* req) -> void {
                req->ID = i;
                req->TimeNano = i;
            });
            APSARA_TEST_TRUE(success);
            APSARA_TEST_EQUAL(cache2.mRequests.size(), 1);
            success = cache2.InsertResp(i, [&](TestResp* resp) -> void {
                resp->ID = i;
                resp->TimeNano = i;
            });
            APSARA_TEST_TRUE(success);
            APSARA_TEST_EQUAL(cache2.mResponses.size(), 0);
            APSARA_TEST_EQUAL(cache2.mRequests.size(), 0);
            APSARA_TEST_EQUAL(cache2.mRespManager.mUnUsed.size(), 0);
            APSARA_TEST_EQUAL(cache2.mReqManager.mUnUsed.size(), 1);
        }
    }

    void TestCommonMapCacheCommon() {
        TestMapCache cache(nullptr);
        for (int i = 0, j = 4; i < 5; ++i, --j) {
            auto success = cache.InsertResp(i, [&](TestResp* resp) -> void {
                resp->ID = i;
                resp->TimeNano = i;
            });
            APSARA_TEST_TRUE(success);
            success = cache.InsertReq(j, [&](TestReq* req) -> void {
                req->ID = i;
                req->TimeNano = i;
            });
            APSARA_TEST_TRUE(success);
            if (i < 2) {
                APSARA_TEST_EQUAL(cache.mRequests.size(), i + 1);
                APSARA_TEST_EQUAL(cache.mResponses.size(), i + 1);
            }
            if (j < 2) {
                APSARA_TEST_EQUAL(cache.mRequests.size(), j);
                APSARA_TEST_EQUAL(cache.mResponses.size(), j);
            }
        }
    }
};


APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestReadUntil, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestSlsStringPiece, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonCacheContinueReq, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonCacheContinueResp, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonCacheContinueOneByOneAndReqFirst, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonCacheContinueOneByOneAndRespFirst, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonCacheInsertOldResp, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonCacheInsertNewReq, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonCacheTryMatchingReq, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonMapCacheContinueReq, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonMapCacheContinueResp, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonMapCacheOneByOne, 0);
APSARA_UNIT_TEST_CASE(ProtocolUtilUnittest, TestCommonMapCacheCommon, 0);
} // namespace logtail


int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}