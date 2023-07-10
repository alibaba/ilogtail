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

#include "network/protocols/utils.h"
#include "observer/network/NetworkObserver.h"
#include "observer/network/ProcessObserver.h"
#include "network/protocols/dns/inner_parser.h"
#include "unittest/Unittest.h"
#include "RawNetPacketReader.h"
#include "unittest/UnittestHelper.h"
#include "network/protocols/dubbo2/inner_parser.h"

namespace logtail {

class ProtocolDubboUnittest : public ::testing::Test {
public:
    void TestHessianRequest() {
        const std::string hexString
            = "dabbc20000000000000000000000011305322e302e32302c6f72672e6170616368652e647562626f2e737072696e67626f6f742e"
              "64656d6f2e44656d6f5365727669636505302e302e300873617948656c6c6f124c6a6176612f6c616e672f537472696e673b0577"
              "6f726c64480470617468302c6f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d6f2e44656d6f536572"
              "766963651272656d6f74652e6170706c69636174696f6e1e647562626f2d737072696e67626f6f742d64656d6f2d636f6e73756d"
              "657209696e74657266616365302c6f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d6f2e44656d6f53"
              "6572766963650776657273696f6e05302e302e300774696d656f7574cbe85a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::NormalMsgType);
        APSARA_TEST_EQUAL(dubbo.mData.ReqId, 0);
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqVersion == "2.0.2");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqServiceName == "org.apache.dubbo.springboot.demo.DemoService");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqServiceVersionName == "0.0.0");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqMethodName == "sayHello");
    }

    void TestHessianResponse() {
        const std::string hexString
            = "dabb021400000000000000000000001b940b48656c6c6f20776f726c644805647562626f05322e302e325a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::NormalMsgType);
        APSARA_TEST_EQUAL(dubbo.mData.ReqId, 0);
        APSARA_TEST_TRUE(dubbo.mData.Response.RespStatus == 20);
    }


    void TestHeassianHeartRequest() {
        const std::string hexString = "dabbe2000000000000000001000000014e";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::HeartbeatType);
    }

    void TestHeassianHeartResponse() {
        const std::string hexString = "dabb22140000000000000001000000014e";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::HeartbeatType);
    }

    void TestFastjsonRequest() {
        const std::string rawHex
            = "dabbc60000000000000000010000013122322e302e32220a226f72672e6170616368652e647562626f2e737072696e67626f6f74"
              "2e64656d6f2e44656d6f53657276696365220a22302e302e30220a2273617948656c6c6f220a224c6a6176612f6c616e672f5374"
              "72696e673b220a22776f726c64220a7b2270617468223a226f72672e6170616368652e647562626f2e737072696e67626f6f742e"
              "64656d6f2e44656d6f53657276696365222c2272656d6f74652e6170706c69636174696f6e223a22647562626f2d737072696e67"
              "626f6f742d64656d6f2d636f6e73756d6572222c22696e74657266616365223a226f72672e6170616368652e647562626f2e7370"
              "72696e67626f6f742e64656d6f2e44656d6f53657276696365222c2276657273696f6e223a22302e302e30222c2274696d656f75"
              "74223a313030307d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(rawHex, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::NormalMsgType);
        APSARA_TEST_EQUAL(dubbo.mData.ReqId, 1);
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqVersion == "2.0.2");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqServiceName == "org.apache.dubbo.springboot.demo.DemoService");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqServiceVersionName == "0.0.0");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqMethodName == "sayHello");
    }

    void TestFastjsonResponse() {
        std::string rawHex
            = "dabb0614000000000000000100000022340a2248656c6c6f20776f726c64220a7b22647562626f223a22322e302e32227d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(rawHex, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::NormalMsgType);
        APSARA_TEST_EQUAL(dubbo.mData.ReqId, 1);
        APSARA_TEST_TRUE(dubbo.mData.Response.RespStatus == 20);
    }


    void TestMetaRequest() {
        std::string rawHex
            = "dabbc2000000000000000000000000ec05322e302e3230296f72672e6170616368652e647562626f2e6d657461646174612e4d65"
              "7461646174615365727669636505312e302e300f6765744d65746164617461496e666f124c6a6176612f6c616e672f537472696e"
              "673b3020633661376530386332353966626437663062663266383261373366393136353248047061746830296f72672e61706163"
              "68652e647562626f2e6d657461646174612e4d65746164617461536572766963650776657273696f6e05312e302e300774696d65"
              "6f7574d413880567726f75701e647562626f2d737072696e67626f6f742d64656d6f2d70726f76696465725a";
        std::vector<uint8_t> data;
        hexstring_to_bin(rawHex, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::NormalMsgType);
        APSARA_TEST_EQUAL(dubbo.mData.ReqId, 0);
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqVersion == "2.0.2");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqServiceName == "org.apache.dubbo.metadata.MetadataService");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqServiceVersionName == "1.0.0");
        APSARA_TEST_TRUE(dubbo.mData.Request.ReqMethodName == "getMetadataInfo");
    }

    void TestMetaResponse() {
        std::string rawHex
            = "dabb02140000000000000000000005fb944330266f72672e6170616368652e647562626f2e6d657461646174612e4d6574616461"
              "7461496e666f93087365727669636573087265766973696f6e03617070604d30266a6176612e7574696c2e636f6e63757272656e"
              "742e436f6e63757272656e74486173684d617030326f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d"
              "6f2e44656d6f536572766963653a647562626f4330326f72672e6170616368652e647562626f2e6d657461646174612e4d657461"
              "64617461496e666f2453657276696365496e666f9606706172616d7304706174680870726f746f636f6c0776657273696f6e0567"
              "726f7570046e616d65614804736964650870726f76696465720772656c6561736505332e302e37076d6574686f64731673617948"
              "656c6c6f2c73617948656c6c6f4173796e630a646570726563617465640566616c736505647562626f05322e302e3209696e7465"
              "7266616365302c6f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d6f2e44656d6f5365727669636514"
              "736572766963652d6e616d652d6d617070696e6704747275650767656e657269630566616c73650d73657269616c697a6174696f"
              "6e08666173746a736f6e0b6170706c69636174696f6e1e647562626f2d737072696e67626f6f742d64656d6f2d70726f76696465"
              "720a6261636b67726f756e640566616c73650764796e616d696304747275651052454749535452595f434c55535445520b7a6b2d"
              "726567697374727907616e79686f737404747275655a302c6f72672e6170616368652e647562626f2e737072696e67626f6f742e"
              "64656d6f2e44656d6f5365727669636505647562626f4e4e302c6f72672e6170616368652e647562626f2e737072696e67626f6f"
              "742e64656d6f2e44656d6f536572766963653054647562626f2d737072696e67626f6f742d64656d6f2d70726f76696465722f6f"
              "72672e6170616368652e647562626f2e6d657461646174612e4d65746164617461536572766963653a312e302e303a647562626f"
              "614804736964650870726f76696465720772656c6561736505332e302e37076d6574686f647330b36765744d6574616461746155"
              "524c2c69734d65746164617461536572766963652c6765744578706f7274656455524c732c736572766963654e616d652c766572"
              "73696f6e2c6765745375627363726962656455524c732c6765744578706f727465645365727669636555524c732c6765744d6574"
              "6164617461496e666f2c746f536f72746564537472696e67732c6765744d65746164617461496e666f732c676574536572766963"
              "65446566696e6974696f6e0a646570726563617465640566616c736505647562626f05322e302e3209696e746572666163653029"
              "6f72672e6170616368652e647562626f2e6d657461646174612e4d657461646174615365727669636514736572766963652d6e61"
              "6d652d6d617070696e6704747275650776657273696f6e05312e302e300767656e657269630566616c7365087265766973696f6e"
              "05332e302e370564656c617901300b6170706c69636174696f6e1e647562626f2d737072696e67626f6f742d64656d6f2d70726f"
              "76696465720a6261636b67726f756e640566616c73650764796e616d69630474727565086578656375746573033130300b636f6e"
              "6e656374696f6e7301311052454749535452595f434c55535445520b7a6b2d72656769737472790567726f75701e647562626f2d"
              "737072696e67626f6f742d64656d6f2d70726f766964657207616e79686f737404747275655a30296f72672e6170616368652e64"
              "7562626f2e6d657461646174612e4d657461646174615365727669636505647562626f05312e302e301e647562626f2d73707269"
              "6e67626f6f742d64656d6f2d70726f766964657230296f72672e6170616368652e647562626f2e6d657461646174612e4d657461"
              "64617461536572766963655a302063366137653038633235396662643766306266326638326137336639313635321e647562626f"
              "2d737072696e67626f6f742d64656d6f2d70726f76696465724805647562626f05322e302e325a";
        std::vector<uint8_t> data;
        hexstring_to_bin(rawHex, data);
        logtail::DubboParser dubbo((const char*)data.data(), (size_t)data.size());
        dubbo.Parse();
        APSARA_TEST_TRUE(dubbo.mData.Type == DubboMsgType::NormalMsgType);
        APSARA_TEST_EQUAL(dubbo.mData.ReqId, 0);
        APSARA_TEST_TRUE(dubbo.mData.Response.RespStatus == 20);
    }


    void TestDubboPacketReader() {
        std::vector<std::string> rawHexs{rawHex2, rawHex1, rawHex4, rawHex3};
        RawNetPacketReader reader("192.168.0.54", 20880, true, ProtocolType_Dubbo, rawHexs);
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets, true);
        for (auto& packet : packets) {
            // std::cout << PacketEventToString(&packet.at(0), packet.size()) << std::endl;
            mObserver->OnPacketEvent(&packet.at(0), packet.size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);

        std::cout << allData[0].DebugString() << std::endl;
        APSARA_TEST_TRUE(
            UnitTestHelper::LogKeyMatched(&allData[0], "req_domain", "org.apache.dubbo.springboot.demo.DemoService"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "version", "2.0.2"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_resource", "sayHello"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_type", "rpc"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_code", "20"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "count", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::GetLogKey(&allData[0], "latency_ns").first == "0");
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_bytes", "321"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_bytes", "50"));

        APSARA_TEST_TRUE(
            UnitTestHelper::LogKeyMatched(&allData[1], "req_domain", "org.apache.dubbo.metadata.MetadataService"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "version", "2.0.2"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "req_resource", "getMetadataInfo"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "req_type", "rpc"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "resp_code", "20"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "count", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::GetLogKey(&allData[1], "latency_ns").first == "0");
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "req_bytes", "252"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "resp_bytes", "1547"));
    }

    void TestDubboParserGC() {
        INT64_FLAG(sls_observer_network_process_timeout) = 18446744073;
        INT64_FLAG(sls_observer_network_connection_timeout) = 18446744073;
        INT64_FLAG(sls_observer_network_process_no_connection_timeout) = 0;
        BOOL_FLAG(sls_observer_network_protocol_stat) = true;

        NetworkStatistic* networkStatistic = NetworkStatistic::GetInstance();
        ProtocolDebugStatistic* protocolDebugStatistic = ProtocolDebugStatistic::GetInstance();
        mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
        NetworkStatistic::Clear();
        APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);

        {
            std::vector<std::string> rawHexs{rawHex1};
            RawNetPacketReader reader("192.168.0.54", 20880, true, ProtocolType_Dubbo, rawHexs);
            APSARA_TEST_TRUE(reader.OK());
            std::vector<std::string> packets;
            reader.GetAllNetPackets(packets, true);
            for (auto& packet : packets) {
                mObserver->OnPacketEvent(&packet.at(0), packet.size());
            }
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 1);
            std::vector<sls_logs::Log> allData;
            mObserver->FlushOutMetrics(allData);
            APSARA_TEST_EQUAL(allData.size(), 0);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds() - 60000000000000ULL);
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 0);

            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionNum, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionCachedSize, 1);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 2);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionNum, 0);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionCachedSize, 0);
        }
        NetworkStatistic::Clear();
        {
            std::vector<std::string> rawHexs{rawHex2};
            RawNetPacketReader reader("192.168.0.54", 20880, true, ProtocolType_Dubbo, rawHexs);
            APSARA_TEST_TRUE(reader.OK());
            std::vector<std::string> packets;
            reader.GetAllNetPackets(packets, true);
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
            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionNum, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionCachedSize, 1);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 2);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionNum, 0);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mDubboConnectionCachedSize, 0);
        }
    }

    NetworkObserver* mObserver = NetworkObserver::GetInstance();
    std::string rawHex1
        = "02000000450001300000400040060000c0a80031c0a80031dd125190843a0dc4d8e90225801818eb82d500000101080a256f7a951a46"
          "6603dabbc2000000000000000000000000ec05322e302e3230296f72672e6170616368652e647562626f2e6d657461646174612e4d65"
          "7461646174615365727669636505312e302e300f6765744d65746164617461496e666f124c6a6176612f6c616e672f537472696e673b"
          "3020633661376530386332353966626437663062663266383261373366393136353248047061746830296f72672e6170616368652e64"
          "7562626f2e6d657461646174612e4d65746164617461536572766963650776657273696f6e05312e302e300774696d656f7574d41388"
          "0567726f75701e647562626f2d737072696e67626f6f742d64656d6f2d70726f76696465725a";
    std::string rawHex2
        = "020000004500063f0000400040060000c0a80031c0a800315190dd12d8e90225843a0ec0801818e787e400000101080a1a466730256f"
          "7a95dabb02140000000000000000000005fb944330266f72672e6170616368652e647562626f2e6d657461646174612e4d6574616461"
          "7461496e666f93087365727669636573087265766973696f6e03617070604d30266a6176612e7574696c2e636f6e63757272656e742e"
          "436f6e63757272656e74486173684d617030326f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d6f2e4465"
          "6d6f536572766963653a647562626f4330326f72672e6170616368652e647562626f2e6d657461646174612e4d65746164617461496e"
          "666f2453657276696365496e666f9606706172616d7304706174680870726f746f636f6c0776657273696f6e0567726f7570046e616d"
          "65614804736964650870726f76696465720772656c6561736505332e302e37076d6574686f64731673617948656c6c6f2c7361794865"
          "6c6c6f4173796e630a646570726563617465640566616c736505647562626f05322e302e3209696e74657266616365302c6f72672e61"
          "70616368652e647562626f2e737072696e67626f6f742e64656d6f2e44656d6f5365727669636514736572766963652d6e616d652d6d"
          "617070696e6704747275650767656e657269630566616c73650d73657269616c697a6174696f6e08666173746a736f6e0b6170706c69"
          "636174696f6e1e647562626f2d737072696e67626f6f742d64656d6f2d70726f76696465720a6261636b67726f756e640566616c7365"
          "0764796e616d696304747275651052454749535452595f434c55535445520b7a6b2d726567697374727907616e79686f737404747275"
          "655a302c6f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d6f2e44656d6f5365727669636505647562626f"
          "4e4e302c6f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d6f2e44656d6f53657276696365305464756262"
          "6f2d737072696e67626f6f742d64656d6f2d70726f76696465722f6f72672e6170616368652e647562626f2e6d657461646174612e4d"
          "65746164617461536572766963653a312e302e303a647562626f614804736964650870726f76696465720772656c6561736505332e30"
          "2e37076d6574686f647330b36765744d6574616461746155524c2c69734d65746164617461536572766963652c6765744578706f7274"
          "656455524c732c736572766963654e616d652c76657273696f6e2c6765745375627363726962656455524c732c6765744578706f7274"
          "65645365727669636555524c732c6765744d65746164617461496e666f2c746f536f72746564537472696e67732c6765744d65746164"
          "617461496e666f732c67657453657276696365446566696e6974696f6e0a646570726563617465640566616c736505647562626f0532"
          "2e302e3209696e7465726661636530296f72672e6170616368652e647562626f2e6d657461646174612e4d6574616461746153657276"
          "69636514736572766963652d6e616d652d6d617070696e6704747275650776657273696f6e05312e302e300767656e65726963056661"
          "6c7365087265766973696f6e05332e302e370564656c617901300b6170706c69636174696f6e1e647562626f2d737072696e67626f6f"
          "742d64656d6f2d70726f76696465720a6261636b67726f756e640566616c73650764796e616d69630474727565086578656375746573"
          "033130300b636f6e6e656374696f6e7301311052454749535452595f434c55535445520b7a6b2d72656769737472790567726f75701e"
          "647562626f2d737072696e67626f6f742d64656d6f2d70726f766964657207616e79686f737404747275655a30296f72672e61706163"
          "68652e647562626f2e6d657461646174612e4d657461646174615365727669636505647562626f05312e302e301e647562626f2d7370"
          "72696e67626f6f742d64656d6f2d70726f766964657230296f72672e6170616368652e647562626f2e6d657461646174612e4d657461"
          "64617461536572766963655a302063366137653038633235396662643766306266326638326137336639313635321e647562626f2d73"
          "7072696e67626f6f742d64656d6f2d70726f76696465724805647562626f05322e302e325a";
    std::string rawHex3
        = "02000000450001750000400040060000c0a80031c0a80031dd1051900ba1972a64669d94801818eb831a00000101080afc1664acac3f"
          "57e4dabbc60000000000000000010000013122322e302e32220a226f72672e6170616368652e647562626f2e737072696e67626f6f74"
          "2e64656d6f2e44656d6f53657276696365220a22302e302e30220a2273617948656c6c6f220a224c6a6176612f6c616e672f53747269"
          "6e673b220a22776f726c64220a7b2270617468223a226f72672e6170616368652e647562626f2e737072696e67626f6f742e64656d6f"
          "2e44656d6f53657276696365222c2272656d6f74652e6170706c69636174696f6e223a22647562626f2d737072696e67626f6f742d64"
          "656d6f2d636f6e73756d6572222c22696e74657266616365223a226f72672e6170616368652e647562626f2e737072696e67626f6f74"
          "2e64656d6f2e44656d6f53657276696365222c2276657273696f6e223a22302e302e30222c2274696d656f7574223a313030307d0a";
    std::string rawHex4
        = "02000000450000660000400040060000c0a80031c0a800315190dd1064669d940ba1986b801818e6820b00000101080aac3f5c9ffc16"
          "64acdabb0614000000000000000100000022340a2248656c6c6f20776f726c64220a7b22647562626f223a22322e302e32227d0a";
};

APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestHessianRequest, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestHessianResponse, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestHeassianHeartRequest, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestHeassianHeartResponse, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestFastjsonRequest, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestFastjsonResponse, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestMetaRequest, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestMetaResponse, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestDubboPacketReader, 0);
APSARA_UNIT_TEST_CASE(ProtocolDubboUnittest, TestDubboParserGC, 0);
} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}