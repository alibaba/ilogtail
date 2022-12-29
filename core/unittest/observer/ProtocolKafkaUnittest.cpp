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
#include "unittest/Unittest.h"
#include "RawNetPacketReader.h"
#include "unittest/UnittestHelper.h"
#include "network/protocols/kafka/inner_parser.h"

#define TEST_VARINT(name, len, expect) \
    do { \
        char arr[] = name; \
        logtail::KafkaParser kafka(arr, len); \
        int64_t val = kafka.readVarInt64(false); \
        APSARA_TEST_EQUAL(val, expect); \
    } while (0)

namespace logtail {

class ProtocolKafkaUnittest : public ::testing::Test {
public:
    void TestFetchReqV4() {
        const char arr[]
            = "\xFF\xFF\xFF\xFF\x00\x00\x01\xF4\x00\x00\x00\x01\x03\x20\x00\x00\x00\x00\x00\x00\x01\x00"
              "\x08\x6D\x79\x2D\x74\x6F\x70\x69\x63\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x01\x7E\x00\x10\x00\x00";
        logtail::KafkaParser kafka(arr, 51);
        kafka.mData.Request.Version = 4;
        kafka.parseFetchRequest();
        APSARA_TEST_EQUAL(kafka.mData.Request.Topic.ToString(), "my-topic");
    }

    void TestFetchReqV11() {
        const char arr[]
            = "\xff\xff\xff\xff\x00\x00\x01\xf4\x00\x00\x00\x01\x03\x20\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x00\x00\x01\x00\x11\x71\x75\x69\x63\x6b\x73\x74\x61\x72\x74\x2d\x65\x76\x65"
              "\x6e\x74\x73\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\xff\xff\xff\xff\xff\xff\xff\xff\x00\x10\x00\x00\x00\x00\x00\x00\x00\x00";
        logtail::KafkaParser kafka(arr, 80);
        kafka.mData.Request.Version = 11;
        kafka.parseFetchRequest();
        APSARA_TEST_EQUAL(kafka.mData.Request.Topic.ToString(), "quickstart-events");
    }

    void TestFetchReqV12() {
        const char arr[]
            = "\xff\xff\xff\xff\x00\x00\x01\xf4\x00\x00\x00\x01\x03\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x02\x12\x71\x75\x69\x63\x6b\x73\x74\x61\x72\x74\x2d\x65\x76\x65\x6e\x74\x73\x02\x00"
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff"
              "\xff\xff\xff\xff\x00\x10\x00\x00\x00\x00\x01\x01\x00";
        logtail::KafkaParser kafka(arr, 59);
        kafka.mData.Request.Version = 12;
        kafka.parseFetchRequest();
        APSARA_TEST_EQUAL(kafka.mData.Request.Topic.ToString(), "quickstart-events");
    }

    void TestFetchRespV4() {
        const char arr[]
            = "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x08\x6D\x79\x2D\x74\x6F\x70\x69\x63\x00\x00\x00\x01"
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x7E\x00\x00\x00\x00\x00\x00\x01\x7E\xFF"
              "\xFF\xFF\xFF\x00\x00\x00\x00";
        logtail::KafkaParser kafka(arr, 52);
        kafka.parseFetchResponse(4);
        APSARA_TEST_EQUAL(kafka.mData.Response.Topic.ToString(), "my-topic");
        APSARA_TEST_EQUAL(kafka.mData.Response.PartitionID, 0);
        APSARA_TEST_EQUAL(kafka.mData.Response.Code, 0);
    }


    void TestFetchRespV11() {
        const char arr[]
            = "\x00\x00\x00\x00\x00\x00\x27\xd5\xb6\xd1\x00\x00\x00\x01\x00\x11\x71\x75\x69\x63\x6b\x73\x74"
              "\x61\x72\x74\x2d\x65\x76\x65\x6e\x74\x73\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff"
              "\xff\xff\xff\xff\xff\xff\x00\x00\x01\x71\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x38\x00"
              "\x00\x00\x00\x02\x7e\x35\x4f\xcb\x00\x00\x00\x00\x00\x00\x00\x00\x01\x7a\xb0\x95\x78\xbc\x00"
              "\x00\x01\x7a\xb0\x95\x78\xbc\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00"
              "\x00\x01\x0c\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x38\x00\x00"
              "\x00\x00\x02\x1b\x91\x32\x93\x00\x00\x00\x00\x00\x00\x00\x00\x01\x7a\xb2\x08\x48\x52\x00\x00"
              "\x01\x7a\xb2\x08\x48\x52\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00"
              "\x01\x0c\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x38\x00\x00\x00"
              "\x00\x02\x99\x41\x19\xe9\x00\x00\x00\x00\x00\x00\x00\x00\x01\x7a\xb2\x08\xde\x56\x00\x00\x01"
              "\x7a\xb2\x08\xde\x56\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01"
              "\x0c\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x46\x00\x00\x00\x00"
              "\x02\xa7\x88\x71\xd8\x00\x00\x00\x00\x00\x00\x00\x00\x01\x7a\xb2\x0a\x70\x1d\x00\x00\x01\x7a"
              "\xb2\x0a\x70\x1d\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01\x28"
              "\x00\x00\x00\x01\x1c\x4d\x79\x20\x66\x69\x72\x73\x74\x20\x65\x76\x65\x6e\x74\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x04\x00\x00\x00\x47\x00\x00\x00\x00\x02\x5c\x9d\xc5\x05\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x01\x7a\xb2\x0a\xb7\xe5\x00\x00\x01\x7a\xb2\x0a\xb7\xe5\xff\xff\xff\xff\xff\xff"
              "\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01\x2a\x00\x00\x00\x01\x1e\x4d\x79\x20\x73\x65"
              "\x63\x6f\x6e\x64\x20\x65\x76\x65\x6e\x74\x00";
        logtail::KafkaParser kafka(arr, 460);
        kafka.parseFetchResponse(11);
        APSARA_TEST_EQUAL(kafka.mData.Response.Topic.ToString(), "quickstart-events");
        APSARA_TEST_EQUAL(kafka.mData.Response.PartitionID, 0);
        APSARA_TEST_EQUAL(kafka.mData.Response.Code, 0);
    }

    void TestFetchRespV12() {
        const char arr[]
            = "\x00\x00\x00\x00\x00\x00\x27\xd5\xb6\xd1\x02\x12\x71\x75\x69\x63\x6b\x73\x74\x61\x72\x74\x2d"
              "\x65\x76\x65\x6e\x74\x73\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00"
              "\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x01\xff\xff\xff\xff\xf2\x02\x00\x00"
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x38\x00\x00\x00\x00\x02\x7e\x35\x4f\xcb\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x01\x7a\xb0\x95\x78\xbc\x00\x00\x01\x7a\xb0\x95\x78\xbc\xff\xff\xff\xff\xff"
              "\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01\x0c\x00\x00\x00\x01\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x01\x00\x00\x00\x38\x00\x00\x00\x00\x02\x1b\x91\x32\x93\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x01\x7a\xb2\x08\x48\x52\x00\x00\x01\x7a\xb2\x08\x48\x52\xff\xff\xff\xff\xff\xff"
              "\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01\x0c\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x02\x00\x00\x00\x38\x00\x00\x00\x00\x02\x99\x41\x19\xe9\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x01\x7a\xb2\x08\xde\x56\x00\x00\x01\x7a\xb2\x08\xde\x56\xff\xff\xff\xff\xff\xff\xff"
              "\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01\x0c\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x03\x00\x00\x00\x46\x00\x00\x00\x00\x02\xa7\x88\x71\xd8\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x01\x7a\xb2\x0a\x70\x1d\x00\x00\x01\x7a\xb2\x0a\x70\x1d\xff\xff\xff\xff\xff\xff\xff\xff"
              "\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01\x28\x00\x00\x00\x01\x1c\x4d\x79\x20\x66\x69\x72\x73"
              "\x74\x20\x65\x76\x65\x6e\x74\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x47\x00\x00\x00"
              "\x00\x02\x5c\x9d\xc5\x05\x00\x00\x00\x00\x00\x00\x00\x00\x01\x7a\xb2\x0a\xb7\xe5\x00\x00\x01"
              "\x7a\xb2\x0a\xb7\xe5\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01"
              "\x2a\x00\x00\x00\x01\x1e\x4d\x79\x20\x73\x65\x63\x6f\x6e\x64\x20\x65\x76\x65\x6e\x74\x00\x00"
              "\x00\x00";
        logtail::KafkaParser kafka(arr, 460);
        kafka.parseFetchResponse(12);
        APSARA_TEST_EQUAL(kafka.mData.Response.Topic.ToString(), "quickstart-events");
        APSARA_TEST_EQUAL(kafka.mData.Response.PartitionID, 0);
        APSARA_TEST_EQUAL(kafka.mData.Response.Code, 0);
    }


    void TestProduceReqV7() {
        const char arr[]
            = "\xFF\xFF\x00\x01\x00\x00\x75\x30\x00\x00\x00\x01\x00\x08\x6D\x79\x2D\x74\x6F\x70\x69\x63\x00"
              "\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x5C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x50"
              "\x00\x00\x00\x00\x02\x76\x7C\xA6\x2F\x00\x00\x00\x00\x00\x01\x00\x00\x01\x7C\x29\x89\x9A\xA2"
              "\x00\x00\x01\x7C\x29\x89\x9A\xA2\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00"
              "\x00\x00\x02\x14\x00\x00\x00\x01\x08\x74\x65\x73\x74\x00\x26\x00\x00\x02\x01\x1A\xC2\x48\x6F"
              "\x6C\x61\x2C\x20\x6D\x75\x6E\x64\x6F\x21\x00";
        logtail::KafkaParser kafka(arr, 103);
        kafka.mData.Request.Version = 7;
        kafka.parseProduceRequest();
        APSARA_TEST_EQUAL(kafka.mData.Request.Version, 7);
        APSARA_TEST_EQUAL(kafka.mData.Request.Acks, 1);
        APSARA_TEST_EQUAL(kafka.mData.Request.TimeoutMs, 30000);
        APSARA_TEST_EQUAL(kafka.mData.Request.Topic.ToString(), "my-topic");
    }

    void TestProduceReqV8() {
        const char arr[] = "\xff\xff\x00\x01\x00\x00\x05\xdc\x00\x00\x00\x01\x00\x11\x71\x75\x69\x63\x6b\x73\x74\x61"
                           "\x72\x74\x2d\x65\x76\x65\x6e\x74\x73\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x52\x00"
                           "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x46\xff\xff\xff\xff\x02\xa7\x88\x71\xd8\x00\x00"
                           "\x00\x00\x00\x00\x00\x00\x01\x7a\xb2\x0a\x70\x1d\x00\x00\x01\x7a\xb2\x0a\x70\x1d\xff\xff"
                           "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x01\x28\x00\x00\x00\x01\x1c"
                           "\x4d\x79\x20\x66\x69\x72\x73\x74\x20\x65\x76\x65\x6e\x74\x00";
        logtail::KafkaParser kafka(arr, 59);
        kafka.mData.Request.Version = 8;
        kafka.parseProduceRequest();
        APSARA_TEST_EQUAL(kafka.mData.Request.Version, 8);
        APSARA_TEST_EQUAL(kafka.mData.Request.Acks, 1);
        APSARA_TEST_EQUAL(kafka.mData.Request.TimeoutMs, 1500);
        APSARA_TEST_EQUAL(kafka.mData.Request.Topic.ToString(), "quickstart-events");
    }


    void TestProduceReqV9() {
        const char arr[] = "\x00\x00\x01\x00\x00\x05\xdc\x02\x12\x71\x75\x69\x63\x6b\x73\x74\x61\x72\x74\x2d\x65"
                           "\x76\x65\x6e\x74\x73\x02\x00\x00\x00\x00\x5b\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                           "\x4e\xff\xff\xff\xff\x02\xc0\xde\x91\x11\x00\x00\x00\x00\x00\x00\x00\x00\x01\x7a\x1b\xc8"
                           "\x2d\xaa\x00\x00\x01\x7a\x1b\xc8\x2d\xaa\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
                           "\xff\xff\x00\x00\x00\x01\x38\x00\x00\x00\x01\x2c\x54\x68\x69\x73\x20\x69\x73\x20\x6d\x79"
                           "\x20\x66\x69\x72\x73\x74\x20\x65\x76\x65\x6e\x74\x00\x00\x00\x00";
        logtail::KafkaParser kafka(arr, 59);
        kafka.mData.Request.Version = 9;
        kafka.parseProduceRequest();
        APSARA_TEST_EQUAL(kafka.mData.Request.Version, 9);
        APSARA_TEST_EQUAL(kafka.mData.Request.Acks, 1);
        APSARA_TEST_EQUAL(kafka.mData.Request.TimeoutMs, 1500);
        APSARA_TEST_EQUAL(kafka.mData.Request.Topic.ToString(), "quickstart-events");
    }

    void TestProduceRespV7() {
        const char arr[]
            = "\x00\x00\x00\x01\x00\x08\x6D\x79\x2D\x74\x6F\x70\x69\x63\x00\x00\x00\x01\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x00\x00\x00\x01\xAE\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x00\x00";
        logtail::KafkaParser kafka(arr, 29);
        kafka.parseProduceResponse(7);
        APSARA_TEST_EQUAL(kafka.mData.Response.Topic.ToString(), "my-topic");
        APSARA_TEST_EQUAL(kafka.mData.Response.PartitionID, 1);
        APSARA_TEST_EQUAL(kafka.mData.Response.Code, 0);
    }

    void TestProduceRespV8() {
        const char arr[] = "\x00\x00\x00\x01\x00\x11\x71\x75\x69\x63\x6b\x73\x74\x61\x72\x74\x2d\x65\x76\x65\x6e\x74"
                           "\x73\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\xff\xff\xff"
                           "\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\x00\x00\x00"
                           "\x00";
        logtail::KafkaParser kafka(arr, 67);
        kafka.parseProduceResponse(8);
        APSARA_TEST_EQUAL(kafka.mData.Response.Topic.ToString(), "quickstart-events");
        APSARA_TEST_EQUAL(kafka.mData.Response.PartitionID, 1);
        APSARA_TEST_EQUAL(kafka.mData.Response.Code, 0);
    }

    void TestProduceRespV9() {
        const char arr[] = "\x02\x12\x71\x75\x69\x63\x6b\x73\x74\x61\x72\x74\x2d\x65\x76\x65\x6e\x74\x73\x02\x00"
                           "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00"
                           "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00";
        logtail::KafkaParser kafka(arr, 59);
        kafka.parseProduceResponse(9);
        APSARA_TEST_EQUAL(kafka.mData.Response.Topic.ToString(), "quickstart-events");
        APSARA_TEST_EQUAL(kafka.mData.Response.PartitionID, 1);
        APSARA_TEST_EQUAL(kafka.mData.Response.Code, 0);
    }


    void TestProduceResponse() {
        const std::string hexString = "0000004000000004000212717569636b73746172742d6576656e7473020000000000000000000000"
                                      "000000ffffffffffffffff0000000000000000010000000000000000";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::KafkaParser kafka((const char*)data.data(), (size_t)data.size());
        kafka.ParseResponse(KafkaApiType::Produce, 9);
        APSARA_TEST_EQUAL(kafka.mData.Response.Code, 0);
    }


    void TestReadVarint() {
        TEST_VARINT("\x96\x01", 2, 150);
        TEST_VARINT("\xff\xff\xff\xff\x0f", 5, -1);
        TEST_VARINT("\x80\xC0\xFF\xFF\x0F", 5, -8192);
        TEST_VARINT("\x00", 1, 0);
        TEST_VARINT("\x02", 1, 2);
        TEST_VARINT("\x12", 1, 18);
        TEST_VARINT("\x03", 1, 3);
        TEST_VARINT("\xff\xff\xff\xff\x07", 5, INT32_MAX);
        TEST_VARINT("\x80\x80\x80\x80\x08", 5, INT32_MIN);
    }

    void TestKafkaProduce() {
        std::vector<std::string> rawHexs{
            produceReq,
            produceResp,
        };
        RawNetPacketReader reader("127.0.0.1", 9092, true, ProtocolType_Kafka, rawHexs);
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets, true);
        for (auto& packet : packets) {
            // std::cout << PacketEventToString(&packet.at(0), packet.size()) << std::endl;
            mObserver->OnPacketEvent(&packet.at(0), packet.size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_TRUE(!allData.empty());
        std::cout << allData[0].DebugString() << std::endl;
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_domain", "quickstart-events"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "version", "9"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_resource", ""));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_type", "produce"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_code", "0"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "count", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::GetLogKey(&allData[0], "latency_ns").first != "0");
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_bytes", "139"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_bytes", "68"));
    }


    void TestKafkaFetch() {
        std::vector<std::string> rawHexs{
            fetchReq,
            fetchResp,
        };
        RawNetPacketReader reader("127.0.0.1", 9092, true, ProtocolType_Kafka, rawHexs);
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets, true);
        for (auto& packet : packets) {
            // std::cout << PacketEventToString(&packet.at(0), packet.size()) << std::endl;
            mObserver->OnPacketEvent(&packet.at(0), packet.size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_TRUE(!allData.empty());
        std::cout << allData[0].DebugString() << std::endl;
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_domain", "quickstart-events"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "version", "12"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_resource", ""));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_type", "fetch"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_code", "0"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "count", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::GetLogKey(&allData[0], "latency_ns").first != "0");
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_bytes", "60"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_bytes", "76"));
    }

    NetworkObserver* mObserver = NetworkObserver::GetInstance();
    std::string produceReq
        = "02000000450000bf00004000400600007f0000017f000001e313238489b9fd9202d80e2980189b6afeb300000101080a652b4a8756f3"
          "8d990000008700000009000000790010636f6e736f6c652d70726f647563657200000001000005dc0212717569636b73746172742d65"
          "76656e747302000000004a00000000000000000000003dffffffff02d3f1a876000000000000000001855758a9a5000001855758a9a5"
          "ffffffffffffffffffffffffffff0000000116000000010a746573743300000000";
    std::string produceResp
        = "020000004500007800004000400600007f0000017f0000012384e31302d80e2989b9fe1d8018b515fe6c00000101"
          "080a56f391b0652b4a870000004000000079000212717569636b73746172742d6576656e74730200000000000000"
          "00000000000002ffffffffffffffff0000000000000000010000000000000000";
    std::string fetchReqEmpty = "000000380001000c0000003c0010636f6e736f6c652d636f6e73756d657200ffffffff000001f400000001"
                                "032000000070f634a60000002801010100";
    std::string fetchRespEmpty = "000000110000003c0000000000000070f634a60100";


    std::string fetchReq = "020000004500007000004000400600007f0000017f000001c57e23841d6da4dbf31cd5a98018a0e2fe640000010"
                           "1080a8ce6a0daaae5a23b000000380001000c000002ba0010636f6e736f6c652d636f6e73756d657200ffffffff"
                           "000001f400000001032000000072365b0b0000024e01010100";
    std::string fetchResp = "020000004500008000004000400600007f0000017f0000012384c57ef31cd5a91d6da5178018a5e8fe74000001"
                            "01080aaae5a2b18ce6a0da000000f8000002ba0000000000000072365b0b0212717569636b73746172742d6576"
                            "656e74730200000000000000000000000000200000000000000020000000000000000000ffffffffae01";
};

// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestProduceReqV7, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestProduceReqV8, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestProduceReqV9, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestProduceRespV7, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestProduceRespV8, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestProduceRespV9, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestReadVarint, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestFetchReqV4, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestFetchReqV11, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestFetchReqV12, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestFetchRespV4, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestFetchRespV11, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestFetchRespV12, 0)
// APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestKafkaProduce, 0)
APSARA_UNIT_TEST_CASE(ProtocolKafkaUnittest, TestKafkaFetch, 0)

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}