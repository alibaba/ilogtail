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

#include <gtest/gtest.h>

#include "unittest/Unittest.h"
#include "network/protocols/utils.h"
#include "network/protocols/redis/inner_parser.h"
#include "RawNetPacketReader.h"
#include "unittest/UnittestHelper.h"
#include "observer/network/ProcessObserver.h"

namespace logtail {


class ProtocolRedisUnittest : public ::testing::Test {
public:
    void TestCommonRequest() {
        // set aa 1;
        const std::string hexString = "2a330d0a24330d0a7365740d0a24320d0a61610d0a24320d0a313b0d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::RedisParser redis((const char*)data.data(), (size_t)data.size());
        redis.parse();
        APSARA_TEST_EQUAL(redis.redisData.isError, false);
        APSARA_TEST_EQUAL(redis.redisData.GetCommands(), "set aa 1;");
    }

    void TestCommonResponse() {
        const std::string hexString = "2b4f4b0d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::RedisParser redis((const char*)data.data(), (size_t)data.size());
        redis.parse();
        APSARA_TEST_EQUAL(redis.redisData.isError, false);
        APSARA_TEST_EQUAL(redis.redisData.GetCommands(), "OK");
    }

    void TestErrResponse() {
        const std::string hexString = "2d45525220756e6b6e6f776e20636f6d6d616e64202741414141270d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::RedisParser redis((const char*)data.data(), (size_t)data.size());
        redis.parse();
        APSARA_TEST_EQUAL(redis.redisData.isError, true);
        APSARA_TEST_EQUAL(redis.redisData.GetCommands(), "ERR unknown command 'AAAA'");
    }

    void TestRangeResponse() {
        const std::string hexString = "2a330d0a24330d0a6f6e650d0a24330d0a74776f0d0a24350d0a74687265650d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::RedisParser redis((const char*)data.data(), (size_t)data.size());
        redis.parse();
        APSARA_TEST_EQUAL(redis.redisData.isError, false);
        APSARA_TEST_EQUAL(redis.redisData.GetCommands(), "one two three");
    }

    void TestIntegerResponse() {
        const std::string hexString = "3a340d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::RedisParser redis((const char*)data.data(), (size_t)data.size());
        redis.parse();
        APSARA_TEST_EQUAL(redis.redisData.isError, false);
        APSARA_TEST_EQUAL(redis.redisData.GetCommands(), "4");
    }

    void TestRedisPacketReader() {
        NetworkObserver* mObserver = new NetworkObserver();
        NetworkConfig::GetInstance()->mDetailProtocolSampling.insert(
            std::make_pair(ProtocolType_Redis, std::make_tuple(true, true, 100)));
        std::vector<std::string> rawHexs{rawHex1, rawHex2};
        RawNetPacketReader reader("30.43.120.215", false, ProtocolType_Redis, rawHexs);
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets);
        for (auto& packet : packets) {
            mObserver->OnPacketEvent(&packet.at(0), packet.size());
        }

        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_EQUAL(allData.size(), size_t(2));

        sls_logs::Log* log = &allData[0];
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "query", "set"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "status", "1"));
        log = &allData[1];
        std::cout << log->DebugString() << std::endl;
    }

    void TestRedisPacketReaderUnorder() {
        NetworkObserver* mObserver = new NetworkObserver();
        NetworkConfig::GetInstance()->mDetailProtocolSampling.insert(
            std::make_pair(ProtocolType_Redis, std::make_tuple(true, true, 100)));
        // response first
        std::vector<std::string> rawHexs{rawHex1, rawHex2};
        RawNetPacketReader reader("30.43.120.215", false, ProtocolType_Redis, rawHexs);
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets);
        for (int i = packets.size() - 1; i >= 0; --i) {
            mObserver->OnPacketEvent(&packets[i].at(0), packets[i].size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_EQUAL(allData.size(), size_t(2));

        sls_logs::Log* log = &allData[0];
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "query", "set"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "status", "1"));
        log = &allData[1];
        std::cout << log->DebugString() << std::endl;
    }

    void TestRedisParserGC() {
        INT64_FLAG(sls_observer_network_process_timeout) = 18446744073;
        INT64_FLAG(sls_observer_network_connection_timeout) = 18446744073;
        INT64_FLAG(sls_observer_network_process_no_connection_timeout) = 0;
        BOOL_FLAG(sls_observer_network_protocol_stat) = true;
        NetworkObserver* mObserver = new NetworkObserver();
        NetworkConfig::GetInstance()->mDetailProtocolSampling.insert(
            std::make_pair(ProtocolType_Redis, std::make_tuple(true, true, 100)));
        NetworkStatistic* networkStatistic = NetworkStatistic::GetInstance();
        ProtocolDebugStatistic* protocolDebugStatistic = ProtocolDebugStatistic::GetInstance();
        mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
        NetworkStatistic::Clear();
        APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);
        {
            std::vector<std::string> rawHexs{rawHex1};
            RawNetPacketReader reader("30.43.120.215", false, ProtocolType_Redis, rawHexs);
            APSARA_TEST_TRUE(reader.OK());
            std::vector<std::string> packets;
            reader.GetAllNetPackets(packets);
            for (auto& packet : packets) {
                mObserver->OnPacketEvent(&packet.at(0), packet.size());
            }
            std::vector<sls_logs::Log> allData;
            mObserver->FlushOutMetrics(allData);
            APSARA_TEST_EQUAL(allData.size(), 0);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds() - 60000000000000ULL);
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 0);

            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionNum, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionCachedSize, 1);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 2);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionNum, 0);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionCachedSize, 0);
        }
        NetworkStatistic::Clear();
        {
            std::vector<std::string> rawHexs{rawHex2};
            RawNetPacketReader reader("30.43.120.215", false, ProtocolType_Redis, rawHexs);
            APSARA_TEST_TRUE(reader.OK());
            std::vector<std::string> packets;
            reader.GetAllNetPackets(packets);
            for (auto& packet : packets) {
                mObserver->OnPacketEvent(&packet.at(0), packet.size());
            }
            std::vector<sls_logs::Log> allData;
            mObserver->FlushOutMetrics(allData);
            APSARA_TEST_EQUAL(allData.size(), 0);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds() - 60000000000000ULL);
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 0);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionNum, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionCachedSize, 1);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 2);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionNum, 0);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mRedisConnectionCachedSize, 0);
        }
    }
    void TestRedisRecombineUnorder() {
        NetworkObserver* mObserver = new NetworkObserver();
        NetworkConfig::GetInstance()->mDetailProtocolSampling.insert(
            std::make_pair(ProtocolType_Redis, std::make_tuple(true, true, 100)));
        std::vector<std::string> rawHexs{combineReq, combineResp0, combineResp1, combineResp2};
        RawNetPacketReader reader("30.240.100.128", false, ProtocolType_Redis, rawHexs, {3, 2, 0, 1});
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets);
        for (auto& packet : packets) {
            mObserver->OnPacketEvent(&packet.at(0), packet.size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_EQUAL(allData.size(), size_t(2));
        sls_logs::Log* log = &allData[0];
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "query", "get"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "status", "1"));
        log = &allData[1];
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "request", "{\"cmd\":\"get a\"}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "type", "l7_details"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "protocol", "redis"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            log,
            "response",
            "{\"result\":"
            "\"dfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjs\",\"success\":\"true\"}"));
        std::cout << log->DebugString() << std::endl;
    }

    void TestRedisRecombine() {
        NetworkObserver* mObserver = new NetworkObserver();
        NetworkConfig::GetInstance()->mDetailProtocolSampling.insert(
            std::make_pair(ProtocolType_Redis, std::make_tuple(true, true, 100)));
        std::vector<std::string> rawHexs{
            combineReq, combineResp0, combineResp1, combineResp2, combineReq, combineResp0};
        RawNetPacketReader reader("30.240.100.128", false, ProtocolType_Redis, rawHexs, {0, 1, 2, 3, 4, 5});
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets);
        for (auto& packet : packets) {
            mObserver->OnPacketEvent(&packet.at(0), packet.size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_EQUAL(allData.size(), size_t(2));

        sls_logs::Log* log = &allData[0];
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "query", "get"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "status", "1"));
        log = &allData[1];
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "request", "{\"cmd\":\"get a\"}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "type", "l7_details"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(log, "protocol", "redis"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            log,
            "response",
            "{\"result\":"
            "\"dfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjsdfg;"
            "jgdfkjdlskfjasldfkjsldkfjdflkgdfkgjdkfgjdflkgjdsflkgjdslfgkjdsflgjkdsfgjdlsfgjksdflkgjsdlfgjdsflgkjsdflgjs"
            "dlfgjlsdkfgjdlsfkgjlsdfkgjdslkfgjldskjfglkdsfjgdklsfgjkdslfglsdkfgjlsdkfjgkdlsfgjklsdfgjdkslfgjdlskfgjdslf"
            "gjsdljvklsdfgjlsdfgjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlfkgjdflskgjsdfklgjsdlfgkjsdlfgjsdlkfgjs;dgsdfjgjsdl;"
            "fgjsdlfgjsdl;fgjsdfklgjsdlfgjksdlf;gjdflsjgsdflgjdsfglkjdls;fgjsd;fgjdsfjlgdsklfgjsd;"
            "jgsdjgfsdfglsdfgjksdjgdls;fgjsld;fgjdslfgjsdjf;gjs\",\"success\":\"true\"}"));
        std::cout << log->DebugString() << std::endl;
    }

    const std::string combineReq
        = "00000000040088665a1cadb40800450000480000400040065f741ef064800b7a4c52c1db18eb0a1851030db435f880180bfa9e93"
          "00000101080a2d32ba9a682690cd2a320d0a24330d0a6765740d0a24310d0a610d0a";
    const std::string combineResp0
        = "88665a1cadb40000000004000800450005dca24f40003506c2900b7a4c521ef0648018ebc1db0db435f80a185117801001f52ce10000"
          "0101080a6826a9452d32ba9a24343038390d0a64666b6a646c736b666a61736c64666b6a736c646b666a64666c6b6764666b676a646b"
          "66676a64666c6b676a6473666c6b676a64736c66676b6a6473666c676a6b647366676a646c7366676a6b7364666c6b676a73646c6667"
          "6a6473666c676b6a7364666c676a73646c66676a6c73646b66676a646c73666b676a6c7364666b676a64736c6b66676a6c64736b6a66"
          "676c6b6473666a67646b6c7366676a6b64736c66676c73646b66676a6c73646b666a676b646c7366676a6b6c736466676a646b736c66"
          "676a646c736b66676a64736c66676a73646c6a766b6c736466676a6c736466676a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a"
          "6a6a6a6a6a6a6a6a6a6a6c666b676a64666c736b676a7364666b6c676a73646c66676b6a73646c66676a73646c6b66676a733b646773"
          "64666a676a73646c3b66676a73646c66676a73646c3b66676a7364666b6c676a73646c66676a6b73646c663b676a64666c736a677364"
          "666c676a647366676c6b6a646c733b66676a73643b66676a6473666a6c6764736b6c66676a73643b6a6773646a6766736466676c7364"
          "66676a6b73646a67646c733b66676a736c643b66676a64736c66676a73646a663b676a736466673b6a6764666b6a646c736b666a6173"
          "6c64666b6a736c646b666a64666c6b6764666b676a646b66676a64666c6b676a6473666c6b676a64736c66676b6a6473666c676a6b64"
          "7366676a646c7366676a6b7364666c6b676a73646c66676a6473666c676b6a7364666c676a73646c66676a6c73646b66676a646c7366"
          "6b676a6c7364666b676a64736c6b66676a6c64736b6a66676c6b6473666a67646b6c7366676a6b64736c66676c73646b66676a6c7364"
          "6b666a676b646c7366676a6b6c736466676a646b736c66676a646c736b66676a64736c66676a73646c6a766b6c736466676a6c736466"
          "676a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6c666b676a64666c736b676a7364666b6c676a7364"
          "6c66676b6a73646c66676a73646c6b66676a733b64677364666a676a73646c3b66676a73646c66676a73646c3b66676a7364666b6c67"
          "6a73646c66676a6b73646c663b676a64666c736a677364666c676a647366676c6b6a646c733b66676a73643b66676a6473666a6c6764"
          "736b6c66676a73643b6a6773646a6766736466676c736466676a6b73646a67646c733b66676a736c643b66676a64736c66676a73646a"
          "663b676a736466673b6a6764666b6a646c736b666a61736c64666b6a736c646b666a64666c6b6764666b676a646b66676a64666c6b67"
          "6a6473666c6b676a64736c66676b6a6473666c676a6b647366676a646c7366676a6b7364666c6b676a73646c66676a6473666c676b6a"
          "7364666c676a73646c66676a6c73646b66676a646c73666b676a6c7364666b676a64736c6b66676a6c64736b6a66676c6b6473666a67"
          "646b6c7366676a6b64736c66676c73646b66676a6c73646b666a676b646c7366676a6b6c736466676a646b736c66676a646c736b6667"
          "6a64736c66676a73646c6a766b6c736466676a6c736466676a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a"
          "6a6a6c666b676a64666c736b676a7364666b6c676a73646c66676b6a73646c66676a73646c6b66676a733b64677364666a676a73646c"
          "3b66676a73646c66676a73646c3b66676a7364666b6c676a73646c66676a6b73646c663b676a64666c736a677364666c676a64736667"
          "6c6b6a646c733b66676a73643b66676a6473666a6c6764736b6c66676a73643b6a6773646a6766736466676c736466676a6b73646a67"
          "646c733b66676a736c643b66676a64736c66676a73646a663b676a736466673b6a6764666b6a646c736b666a61736c64666b6a736c64"
          "6b666a64666c6b6764666b676a646b66676a64666c6b676a6473666c6b676a64736c66676b6a6473666c676a6b647366676a646c7366"
          "676a";
    const std::string combineResp1
        = "88665a1cadb40000000004000800450005dca25040003506c28f0b7a4c521ef0648018ebc1db0db43ba00a185117801001f5f2610000"
          "0101080a6826a9452d32ba9a6b7364666c6b676a73646c66676a6473666c676b6a7364666c676a73646c66676a6c73646b66676a646c"
          "73666b676a6c7364666b676a64736c6b66676a6c64736b6a66676c6b6473666a67646b6c7366676a6b64736c66676c73646b66676a6c"
          "73646b666a676b646c7366676a6b6c736466676a646b736c66676a646c736b66676a64736c66676a73646c6a766b6c736466676a6c73"
          "6466676a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6c666b676a64666c736b676a7364666b6c676a"
          "73646c66676b6a73646c66676a73646c6b66676a733b64677364666a676a73646c3b66676a73646c66676a73646c3b66676a7364666b"
          "6c676a73646c66676a6b73646c663b676a64666c736a677364666c676a647366676c6b6a646c733b66676a73643b66676a6473666a6c"
          "6764736b6c66676a73643b6a6773646a6766736466676c736466676a6b73646a67646c733b66676a736c643b66676a64736c66676a73"
          "646a663b676a736466673b6a6764666b6a646c736b666a61736c64666b6a736c646b666a64666c6b6764666b676a646b66676a64666c"
          "6b676a6473666c6b676a64736c66676b6a6473666c676a6b647366676a646c7366676a6b7364666c6b676a73646c66676a6473666c67"
          "6b6a7364666c676a73646c66676a6c73646b66676a646c73666b676a6c7364666b676a64736c6b66676a6c64736b6a66676c6b647366"
          "6a67646b6c7366676a6b64736c66676c73646b66676a6c73646b666a676b646c7366676a6b6c736466676a646b736c66676a646c736b"
          "66676a64736c66676a73646c6a766b6c736466676a6c736466676a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a"
          "6a6a6a6a6c666b676a64666c736b676a7364666b6c676a73646c66676b6a73646c66676a73646c6b66676a733b64677364666a676a73"
          "646c3b66676a73646c66676a73646c3b66676a7364666b6c676a73646c66676a6b73646c663b676a64666c736a677364666c676a6473"
          "66676c6b6a646c733b66676a73643b66676a6473666a6c6764736b6c66676a73643b6a6773646a6766736466676c736466676a6b7364"
          "6a67646c733b66676a736c643b66676a64736c66676a73646a663b676a736466673b6a6764666b6a646c736b666a61736c64666b6a73"
          "6c646b666a64666c6b6764666b676a646b66676a64666c6b676a6473666c6b676a64736c66676b6a6473666c676a6b647366676a646c"
          "7366676a6b7364666c6b676a73646c66676a6473666c676b6a7364666c676a73646c66676a6c73646b66676a646c73666b676a6c7364"
          "666b676a64736c6b66676a6c64736b6a66676c6b6473666a67646b6c7366676a6b64736c66676c73646b66676a6c73646b666a676b64"
          "6c7366676a6b6c736466676a646b736c66676a646c736b66676a64736c66676a73646c6a766b6c736466676a6c736466676a6a6a6a6a"
          "6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6c666b676a64666c736b676a7364666b6c676a73646c66676b6a73"
          "646c66676a73646c6b66676a733b64677364666a676a73646c3b66676a73646c66676a73646c3b66676a7364666b6c676a73646c6667"
          "6a6b73646c663b676a64666c736a677364666c676a647366676c6b6a646c733b66676a73643b66676a6473666a6c6764736b6c66676a"
          "73643b6a6773646a6766736466676c736466676a6b73646a67646c733b66676a736c643b66676a64736c66676a73646a663b676a7364"
          "66673b6a6764666b6a646c736b666a61736c64666b6a736c646b666a64666c6b6764666b676a646b66676a64666c6b676a6473666c6b"
          "676a64736c66676b6a6473666c676a6b647366676a646c7366676a6b7364666c6b676a73646c66676a6473666c676b6a7364666c676a"
          "73646c66676a6c73646b66676a646c73666b676a6c7364666b676a64736c6b66676a6c64736b6a66676c6b6473666a67646b6c736667"
          "6a6b";
    const std::string combineResp2
        = "88665a1cadb40000000004000800450004e6a25140003506c3840b7a4c521ef0648018ebc1db0db441480a185117801801f557a30000"
          "0101080a6826a9452d32ba9a64736c66676c73646b66676a6c73646b666a676b646c7366676a6b6c736466676a646b736c66676a646c"
          "736b66676a64736c66676a73646c6a766b6c736466676a6c736466676a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a"
          "6a6a6a6a6a6a6c666b676a64666c736b676a7364666b6c676a73646c66676b6a73646c66676a73646c6b66676a733b64677364666a67"
          "6a73646c3b66676a73646c66676a73646c3b66676a7364666b6c676a73646c66676a6b73646c663b676a64666c736a677364666c676a"
          "647366676c6b6a646c733b66676a73643b66676a6473666a6c6764736b6c66676a73643b6a6773646a6766736466676c736466676a6b"
          "73646a67646c733b66676a736c643b66676a64736c66676a73646a663b676a736466673b6a6764666b6a646c736b666a61736c64666b"
          "6a736c646b666a64666c6b6764666b676a646b66676a64666c6b676a6473666c6b676a64736c66676b6a6473666c676a6b647366676a"
          "646c7366676a6b7364666c6b676a73646c66676a6473666c676b6a7364666c676a73646c66676a6c73646b66676a646c73666b676a6c"
          "7364666b676a64736c6b66676a6c64736b6a66676c6b6473666a67646b6c7366676a6b64736c66676c73646b66676a6c73646b666a67"
          "6b646c7366676a6b6c736466676a646b736c66676a646c736b66676a64736c66676a73646c6a766b6c736466676a6c736466676a6a6a"
          "6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6c666b676a64666c736b676a7364666b6c676a73646c66676b"
          "6a73646c66676a73646c6b66676a733b64677364666a676a73646c3b66676a73646c66676a73646c3b66676a7364666b6c676a73646c"
          "66676a6b73646c663b676a64666c736a677364666c676a647366676c6b6a646c733b66676a73643b66676a6473666a6c6764736b6c66"
          "676a73643b6a6773646a6766736466676c736466676a6b73646a67646c733b66676a736c643b66676a64736c66676a73646a663b676a"
          "736466673b6a6764666b6a646c736b666a61736c64666b6a736c646b666a64666c6b6764666b676a646b66676a64666c6b676a647366"
          "6c6b676a64736c66676b6a6473666c676a6b647366676a646c7366676a6b7364666c6b676a73646c66676a6473666c676b6a7364666c"
          "676a73646c66676a6c73646b66676a646c73666b676a6c7364666b676a64736c6b66676a6c64736b6a66676c6b6473666a67646b6c73"
          "66676a6b64736c66676c73646b66676a6c73646b666a676b646c7366676a6b6c736466676a646b736c66676a646c736b66676a64736c"
          "66676a73646c6a766b6c736466676a6c736466676a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6c66"
          "6b676a64666c736b676a7364666b6c676a73646c66676b6a73646c66676a73646c6b66676a733b64677364666a676a73646c3b66676a"
          "73646c66676a73646c3b66676a7364666b6c676a73646c66676a6b73646c663b676a64666c736a677364666c676a647366676c6b6a64"
          "6c733b66676a73643b66676a6473666a6c6764736b6c66676a73643b6a6773646a6766736466676c736466676a6b73646a67646c733b"
          "66676a736c643b66676a64736c66676a73646a663b676a730d0a";


    const std::string rawHex1
        = "00749c945d39a07817a0852e08004500005b000040004006375a1e2b78d70b9f60a2e2ae18ebeec9ad5886a8e17980180800c6e80000"
          "0101080a4d49243fd112ef9f2a330d0a24330d0a7365740d0a24310d0a610d0a2431320d0a6861686167617367667361660d0a";
    const std::string rawHex2 = "a07817a0852e00749c945d39080045000039b7464000370689350b9f60a21e2b78d718ebe2ae86a8e179ee"
                                "c9ad7f80180039a65a00000101080ad11409da4d49243f2b4f4b0d0a";
};

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestCommonRequest, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestCommonResponse, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestErrResponse, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestRangeResponse, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestIntegerResponse, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestRedisPacketReader, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestRedisPacketReaderUnorder, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestRedisParserGC, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestRedisRecombine, 0);

APSARA_UNIT_TEST_CASE(ProtocolRedisUnittest, TestRedisRecombineUnorder, 0);

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}