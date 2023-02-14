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
#include "network/protocols/utils.h"
#include "network/protocols/http/inner_parser.h"
#include "RawNetPacketReader.h"
#include "network/protocols/http/parser.h"
#include "observer/network/ProcessObserver.h"
#include "unittest/UnittestHelper.h"

namespace logtail {


class ProtocolHttpUnittest : public ::testing::Test {
public:
    std::string getHeaders(logtail::HTTPParser& http, std::string name) {
        auto res = http.GetPacket().Common.Headers.find(name);
        if (res != http.GetPacket().Common.Headers.end()) {
            return std::string(res->second.data(), res->second.size());
        }
        return "";
    }

    // POST /a HTTP/1.1\r\n
    //     [Expert Info (Chat/Sequence): POST /a HTTP/1.1\r\n]
    //     Request Method: POST
    //     Request URI: /a
    //     Request Version: HTTP/1.1
    // Host: ocs-oneagent-server.alibaba.com\r\n
    // Content-Type: application/json\r\n
    // Connection: keep-alive\r\n
    // Accept: */*\r\n
    // User-Agent: CloudShell/5.0.42 (Mac OS X Version 12.2.1 (Build 21D62))\r\n
    // Accept-Language: zh-Hans-CN;q=1\r\n
    // Accept-Encoding: gzip, deflate\r\n
    // Content-Length: 1475\r\n
    //     [Content length: 1475]
    // \r\n
    // [Full request URI: http://ocs-oneagent-server.alibaba.com/a]
    // [HTTP request 3/24]
    // [Prev request in frame: 2962]
    // [Response in frame: 4580]
    // [Next request in frame: 5847]
    // File Data: 1475 bytes
    void TestCommonRequest() {
        const std::string hexString
            = "504f5354202f6120485454502f312e310d0a486f73743a206f63732d6f6e656167656e742d7365727665722e616c69626162612e"
              "636f6d0d0a436f6e74656e742d547970653a206170706c69636174696f6e2f6a736f6e0d0a436f6e6e656374696f6e3a206b6565"
              "702d616c6976650d0a4163636570743a202a2f2a0d0a557365722d4167656e743a20436c6f75645368656c6c2f352e302e343220"
              "284d6163204f5320582056657273696f6e2031322e322e3120284275696c6420323144363229290d0a4163636570742d4c616e67"
              "756167653a207a682d48616e732d434e3b713d310d0a4163636570742d456e636f64696e673a20677a69702c206465666c617465"
              "0d0a436f6e74656e742d4c656e6774683a20313437350d0a0d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::HTTPParser http((const char*)data.data(), (size_t)data.size());
        auto res = http.ParseRequest();
        APSARA_TEST_EQUAL(res, ParseResult_Partial);
        APSARA_TEST_EQUAL(std::string(http.GetPacket().Req.Method, http.GetPacket().Req.MethodLen), "POST");
        APSARA_TEST_EQUAL(std::string(http.GetPacket().Req.Url, http.GetPacket().Req.UrlLen), "/a");
        APSARA_TEST_TRUE(http.GetPacket().Common.Version == 1);
        APSARA_TEST_TRUE(http.GetPacket().Common.RawHeadersNum == 8);
        APSARA_TEST_EQUAL(getHeaders(http, "Host"), "ocs-oneagent-server.alibaba.com");
        APSARA_TEST_EQUAL(getHeaders(http, "Content-Type"), "application/json");
        APSARA_TEST_EQUAL(getHeaders(http, "Connection"), "keep-alive");
        APSARA_TEST_EQUAL(getHeaders(http, "Accept"), "*/*");
        APSARA_TEST_EQUAL(getHeaders(http, "User-Agent"), "CloudShell/5.0.42 (Mac OS X Version 12.2.1 (Build 21D62))");
        APSARA_TEST_EQUAL(getHeaders(http, "Accept-Language"), "zh-Hans-CN;q=1");
        APSARA_TEST_EQUAL(getHeaders(http, "Accept-Encoding"), "gzip, deflate");
        APSARA_TEST_EQUAL(getHeaders(http, "Content-Length"), "1475");
    }


    void TestCommonRequest2() {
        const std::string hexString
            = "504f5354202f6c6f6773746f7265732f6c6f677461696c5f7374617475735f70726f66696c652f7368617264732f6c6220485454"
              "502f312e310d0a486f73743a616c692d636e2d7368616e676861692d636f72702d736c732d61646d696e2e636e2d7368616e6768"
              "61692d636f72702e736c732e616c6979756e63732e636f6d0d0a4163636570743a202a2f2a0d0a417574686f72697a6174696f6e"
              "3a4c4f47203934746f337a3431387975706936696b61777171643337303a69544b46686c36736f6e6a6a715447306437356d5433"
              "474d6438413d0d0a436f6e74656e742d4c656e6774683a313135340d0a436f6e74656e742d4d44353a3637463146384143333136"
              "4137383238304342304131343037313937464344340d0a436f6e74656e742d547970653a6170706c69636174696f6e2f782d7072"
              "6f746f6275660d0a446174653a5475652c203239204d617220323032322031313a32353a313620474d540d0a557365722d416765"
              "6e743a616c692d6c6f672d6c6f677461696c0d0a782d6c6f672d61706976657273696f6e3a302e362e300d0a782d6c6f672d626f"
              "647972617773697a653a313738300d0a782d6c6f672d636f6d7072657373747970653a6c7a340d0a782d6c6f672d6b657970726f"
              "76696465723a6d64352d736861312d73616c740d0a782d6c6f672d7369676e61747572656d6574686f643a686d61632d73686131"
              "0d0a4578706563743a203130302d636f6e74696e75650d0a0d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::HTTPParser http((const char*)data.data(), (size_t)data.size());
        auto res = http.ParseRequest();
        APSARA_TEST_TRUE(res == ParseResult_Partial);
        APSARA_TEST_EQUAL(std::string(http.GetPacket().Req.Method, http.GetPacket().Req.MethodLen), "POST");
        APSARA_TEST_EQUAL(std::string(http.GetPacket().Req.Url, http.GetPacket().Req.UrlLen),
                          "/logstores/logtail_status_profile/shards/lb");
        APSARA_TEST_TRUE(http.GetPacket().Common.Version == 1);
        APSARA_TEST_TRUE(http.GetPacket().Common.RawHeadersNum == 14);

        APSARA_TEST_EQUAL(getHeaders(http, "Date"), "Tue, 29 Mar 2022 11:25:16 GMT");
        APSARA_TEST_EQUAL(getHeaders(http, "Host"), "ali-cn-shanghai-corp-sls-admin.cn-shanghai-corp.sls.aliyuncs.com");
        APSARA_TEST_EQUAL(getHeaders(http, "Accept"), "*/*");
        APSARA_TEST_EQUAL(getHeaders(http, "Expect"), "100-continue");
        APSARA_TEST_EQUAL(getHeaders(http, "User-Agent"), "ali-log-logtail");
        APSARA_TEST_EQUAL(getHeaders(http, "Content-Type"), "application/x-protobuf");
    }


    //        HTTP/1.1 200 OK\r\n
    //                Server: Tengine\r\n
    //                Content-Length: 0\r\n
    //                Connection: close\r\n
    //                Access-Control-Allow-Origin: *\r\n
    //                Date: Sun, 19 Jun 2022 13:36:44 GMT\r\n
    //                x-log-append-meta: true\r\n
    //                x-log-time: 1655645804\r\n
    //                x-log-requestid: 62AF266CFF534E3CBFEB797C\r\n
    //        \r\n
    //        [HTTP response 2/2]
    //        [Time since request: 0.015786000 seconds]
    //        [Prev response in frame: 97700]
    //        [Request in frame: 97703]
    //        [Request URI:
    //        http://edr-project.cn-beijing.log.aliyuncs.com/logstores/bradar_performance_statistics_log/shards/lb]
    void TestCommonResponse() {
        const std::string hexString
            = "485454502f312e3120323030204f4b0d0a5365727665723a2054656e67696e650d0a436f6e74656e742d4c656e6774683a20300d"
              "0a436f6e6e656374696f6e3a20636c6f73650d0a4163636573732d436f6e74726f6c2d416c6c6f772d4f726967696e3a202a0d0a"
              "446174653a2053756e2c203139204a756e20323032322031333a33363a343420474d540d0a782d6c6f672d617070656e642d6d65"
              "74613a20747275650d0a782d6c6f672d74696d653a20313635353634353830340d0a782d6c6f672d7265717565737469643a2036"
              "32414632363643464635333445334342464542373937430d0a0d0a";
        std::vector<uint8_t> data;
        hexstring_to_bin(hexString, data);
        logtail::HTTPParser http((const char*)data.data(), (size_t)data.size());
        auto res = http.ParseResp();
        APSARA_TEST_EQUAL(res, ParseResult::ParseResult_OK);
        APSARA_TEST_EQUAL(http.GetPacket().Common.Version, 1);
        APSARA_TEST_EQUAL(http.GetPacket().Resp.Code, 200);
        APSARA_TEST_EQUAL(http.GetPacket().Common.RawHeadersNum, 8);
        APSARA_TEST_EQUAL(getHeaders(http, "Server"), "Tengine");
        APSARA_TEST_EQUAL(getHeaders(http, "Content-Length"), "0");
        APSARA_TEST_EQUAL(getHeaders(http, "Connection"), "close");
        APSARA_TEST_EQUAL(getHeaders(http, "Access-Control-Allow-Origin"), "*");
        APSARA_TEST_EQUAL(getHeaders(http, "x-log-append-meta"), "true");
        APSARA_TEST_EQUAL(getHeaders(http, "x-log-time"), "1655645804");
        APSARA_TEST_EQUAL(getHeaders(http, "x-log-requestid"), "62AF266CFF534E3CBFEB797C");
    }

    void TestHTTPPacketReaderOrder() {
        NetworkObserver* mObserver = new NetworkObserver();
        NetworkConfig::GetInstance()->mDetailProtocolSampling.insert(
            std::make_pair(ProtocolType_HTTP, std::make_tuple(true, true, 100)));
        std::vector<std::string> rawHexs{req1_0, req1_1, req1_2, resp1_0, resp1_1};
        RawNetPacketReader reader("30.240.100.129", false, ProtocolType_HTTP, rawHexs, {0, 1, 2, 3, 4});
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets);
        for (auto& packet : packets) {
            mObserver->OnPacketEvent(&packet.at(0), packet.size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "version", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_domain", "ocs-oneagent-server.alibaba.com"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_type", "POST"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_resource", "/a"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_code", "200"));
        std::cout << allData[1].DebugString() << std::endl;

        APSARA_TEST_TRUE(
            UnitTestHelper::LogKeyMatched(&allData[1],
                                          "request",
                                          "{\"body\":\"\",\"headers\":{\"Accept\":\"*/*\",\"Accept-Encoding\":\"gzip, "
                                          "deflate\",\"Accept-Language\":\"zh-Hans-CN;q=1\",\"Connection\":\"keep-"
                                          "alive\",\"Content-Length\":\"1575\",\"Content-Type\":\"application/"
                                          "json\",\"Host\":\"ocs-oneagent-server.alibaba.com\",\"User-Agent\":"
                                          "\"CloudShell/8.5.0 (Mac OS X ban ben13.2(ban "
                                          "hao22D49))\"},\"host\":\"ocs-oneagent-server.alibaba.com\",\"method\":"
                                          "\"POST\",\"url\":\"/a\",\"version\":\"1\"}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            &allData[1],
            "response",
            "{\"body\":\"{\\\"encrypted\\\":true,\\\"value\\\":\\\"tw2vWP5pz53AL7aNzHqUeg==\\\"}\",\"code\":200,"
            "\"headers\":{\"Connection\":\"keep-alive\",\"Content-Length\":\"53\",\"Content-Type\":\"application/"
            "json;charset=UTF-8\",\"Date\":\"Fri, 10 Feb 2023 02:33:41 "
            "GMT\",\"EagleEye-TraceId\":\"215046f416759964212893624e7799\",\"Server\":\"Tengine/"
            "Aserver\",\"Timing-Allow-Origin\":\"*\",\"X-Application-Context\":\"ocs-oneagent-server:7001\"}}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "type", "l7_details"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "protocol", "http"));
    }


    void TestHTTPPacketReaderUnorder() {
        NetworkObserver* mObserver = new NetworkObserver();
        std::vector<std::string> rawHexs{req1_0, req1_1, req1_2, resp1_0, resp1_1};
        RawNetPacketReader reader("30.240.100.129", false, ProtocolType_HTTP, rawHexs, {4, 3, 2, 1, 0});
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets);
        for (int i = packets.size() - 1; i >= 0; --i) {
            mObserver->OnPacketEvent(&packets[i].at(0), packets[i].size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "version", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_domain", "ocs-oneagent-server.alibaba.com"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_type", "POST"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_resource", "/a"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_code", "200"));
        std::cout << allData[1].DebugString() << std::endl;

        APSARA_TEST_TRUE(
            UnitTestHelper::LogKeyMatched(&allData[1],
                                          "request",
                                          "{\"body\":\"\",\"headers\":{\"Accept\":\"*/*\",\"Accept-Encoding\":\"gzip, "
                                          "deflate\",\"Accept-Language\":\"zh-Hans-CN;q=1\",\"Connection\":\"keep-"
                                          "alive\",\"Content-Length\":\"1575\",\"Content-Type\":\"application/"
                                          "json\",\"Host\":\"ocs-oneagent-server.alibaba.com\",\"User-Agent\":"
                                          "\"CloudShell/8.5.0 (Mac OS X ban ben13.2(ban "
                                          "hao22D49))\"},\"host\":\"ocs-oneagent-server.alibaba.com\",\"method\":"
                                          "\"POST\",\"url\":\"/a\",\"version\":\"1\"}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            &allData[1],
            "response",
            "{\"body\":\"{\\\"encrypted\\\":true,\\\"value\\\":\\\"tw2vWP5pz53AL7aNzHqUeg==\\\"}\",\"code\":200,"
            "\"headers\":{\"Connection\":\"keep-alive\",\"Content-Length\":\"53\",\"Content-Type\":\"application/"
            "json;charset=UTF-8\",\"Date\":\"Fri, 10 Feb 2023 02:33:41 "
            "GMT\",\"EagleEye-TraceId\":\"215046f416759964212893624e7799\",\"Server\":\"Tengine/"
            "Aserver\",\"Timing-Allow-Origin\":\"*\",\"X-Application-Context\":\"ocs-oneagent-server:7001\"}}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "type", "l7_details"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "protocol", "http"));
    }

    void TestHTTPPacketChunkedAndPost() {
        NetworkObserver* mObserver = new NetworkObserver();
        std::vector<std::string> rawHexs{chunk_req,
                                         chunk_resp_0,
                                         chunk_resp_1,
                                         chunk_resp_2,
                                         chunk_resp_3,
                                         chunk_resp_4,
                                         simple_post,
                                         simple_post_resp};
        std::vector<std::string> respHexs{};
        RawNetPacketReader reader("192.168.0.45", false, ProtocolType_HTTP, rawHexs, {0, 1, 2, 3, 4, 5, 6, 7});
        APSARA_TEST_TRUE(reader.OK());
        std::vector<std::string> packets;
        reader.GetAllNetPackets(packets);
        for (const auto& item : packets) {
            mObserver->OnPacketEvent((void*)&(item.at(0)), item.size());
        }
        std::vector<sls_logs::Log> allData;
        mObserver->FlushOutMetrics(allData);
        mObserver->FlushOutDetails(allData);
        APSARA_TEST_EQUAL(allData.size(), 4);
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "version", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_domain", "anglesharp.azurewebsites.net"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_type", "POST"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "req_resource", "/PostAnything"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[0], "resp_code", "200"));

        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "version", "1"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "req_domain", "anglesharp.azurewebsites.net"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "req_type", "GET"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "req_resource", "/Chunked"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[1], "resp_code", "200"));


        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            &allData[2],
            "request",
            "{\"body\":\"\",\"headers\":{\"Accept\":\"text/html,application/xhtml+xml,application/xml;q=0.9,image/"
            "avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\",\"Accept-Encoding\":\"gzip, "
            "deflate\",\"Accept-Language\":\"zh,zh-CN;q=0.9\",\"Connection\":\"keep-alive\",\"Host\":\"anglesharp."
            "azurewebsites.net\",\"Upgrade-Insecure-Requests\":\"1\",\"User-Agent\":\"Mozilla/5.0 (Macintosh; Intel "
            "Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/110.0.0.0 "
            "Safari/537.36\"},\"host\":\"anglesharp.azurewebsites.net\",\"method\":\"GET\",\"url\":\"/"
            "Chunked\",\"version\":\"1\"}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            &allData[2],
            "response",
            "{\"body\":\"\",\"code\":200,\"headers\":{\"Cache-Control\":\"private\",\"Content-Encoding\":\"gzip\","
            "\"Content-Type\":\"text/html\",\"Date\":\"Sat, 11 Feb 2023 14:30:35 "
            "GMT\",\"Server\":\"Microsoft-IIS/"
            "10.0\",\"Set-Cookie\":\"ARRAffinity=88eb07764da9fb4879ac3c8215c73aa6e77bd078ebc0fe627e51d6db747a12ec;Path="
            "/;HttpOnly;Domain=anglesharp.azurewebsites.net\",\"Transfer-Encoding\":\"chunked\",\"Vary\":\"Accept-"
            "Encoding\",\"X-AspNet-Version\":\"4.0.30319\",\"X-AspNetMvc-Version\":\"5.0\",\"X-Powered-By\":\"ASP."
            "NET\"}}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[2], "protocol", "http"));

        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            &allData[3],
            "request",
            "{\"body\":\"\",\"headers\":{\"Accept\":\"text/html,application/xhtml+xml,application/xml;q=0.9,image/"
            "avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\",\"Accept-Encoding\":\"gzip, "
            "deflate\",\"Accept-Language\":\"zh,zh-CN;q=0.9\",\"Cache-Control\":\"max-age=0\",\"Connection\":\"keep-"
            "alive\",\"Content-Length\":\"135\",\"Content-Type\":\"application/"
            "x-www-form-urlencoded\",\"Cookie\":\"__RequestVerificationToken="
            "C0fVRGqxAUQEhmR6cbKEjOtiteYSPCz2wwWtlli7XpFrHZ4p1M0k6BU6aASY1KdkWrf35dnUPTIU7_FhH9zhZMVNjShy5Qz8zEmc_"
            "pTm5XE1\",\"Host\":\"anglesharp.azurewebsites.net\",\"Origin\":\"http://"
            "anglesharp.azurewebsites.net\",\"Referer\":\"http://anglesharp.azurewebsites.net/"
            "PostAnything\",\"Upgrade-Insecure-Requests\":\"1\",\"User-Agent\":\"Mozilla/5.0 (Macintosh; Intel Mac OS "
            "X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/110.0.0.0 "
            "Safari/537.36\"},\"host\":\"anglesharp.azurewebsites.net\",\"method\":\"POST\",\"url\":\"/"
            "PostAnything\",\"version\":\"1\"}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(
            &allData[3],
            "response",
            "{\"body\":\"\\u001f\\u02c8\\u0000\\u0000\\u0000\\u0000\\u0000\\u0004\\u0000\\ufffd`\\u001cI\\u05a5&/"
            "m\\u02bb\177J\\u17e9\\udde0t\\ufffd\\ufffd\\u0013$\\u0610@\\u0010\\uc048\\u0366\\u04ac\\u001diG#)"
            "\\u02ea\\ufffdeVe]f\\u0016@"
            "\\u032d\\u077c\\u983b\\udeef\\u0777\\u07bb\\uff77\\u06bb\\u074e\'\\u983f\\udfff\\\\fd\\u0001l\\u57f8\\ude9"
            "a\\u025e!\\ufffd\\u021f?~|\\u001f?\\\"\\u001e\\ufffd\\u038f\\u07fcy\\u98af\\udf3cM\\u7b62<"
            "\\ufffd\\u0353\\u01f6g\\u078d\\ud88d\\udc77Y:\\u0767u\\u04f7\\u07fd\\u052e\\u03f7\\u000f>"
            "J\\uf89b\\u05a8\\u02fc\\u8955\\u0507\\u02ebv^,/"
            "\\u001e\\u07d5\\u03e8\\u02fb\\u6f495\\u06e6\\u07f8\\ufc7c\\u05d3\\u073e\\ufffd\\u03ebz\\u0466\\u04f6\\u021"
            "6\\u07fdt\\u05ef\\u1804\\ude9d\\u03eb\\u0667\\u001f\\u0368\\u04cf\\u039e\\u0017\\u02d5\\u068d\\u05d9\\\"\\"
            "ufffd\\uc8df\\ufffd\\u98bd\\udd7f\\u00b5\\u07b4?\\u0657\\u01791\\u0340\\ufffd6_~\\u0536\\u05ebj1/"
            "f3\\ufffdu\\u0655k\\ufffd3{\\ufffd\\ufffd\\u0728?"
            "m\\u05db\\u03fe\\u5827\\ude7d\\u5875\\udf5d\\u02c5\\u05ff\\\\\\ueb1d\\ufffd`~U]/"
            "~\\uf574\\u98ab\\udf2c\'\\u0016\\u07fd\\u027c\\u07b1\\ueaec\\u180e\\uddff\\u180e\\udffeX?\\u0777Sof?"
            "\\u021e|\\u069e\177y\\u073bS?\\u1841\\uddfd`"
            "\\ufffd\\u02de\\ufffd\\u426a\\ufffd\\u2d6bU\\ufffdU\\ufffdb\\ufffd{\\uff3e\\ufffd\\u01ae\177\\ufffd|"
            "\\uf9de6\\u0017\\u06e0QJ\\u03e3Yq\\u064e\\u02eci>"
            "\\ufffd\\b\\u371eWu\\u17ce\\ude99f\\u51f4vj\\ufffd\\u3e5e\\u04594\\u0465/"
            "\\ufffd\\u02a6X\\ufffd\\ufffd\\ufffd\\u00ec\\u0228u\\u07acK\\\"\\u02fb\\n\\u000f\177\\u035d\\u0016m\\u07a8"
            "~\\u03e7q\\u04ca\\u05a6\\u045b\\ufffd\\ufffd\\u00e3aj>\\u078b\\u01b7\\u0001a\\u001a\177t\\u053dp^~"
            "\\ufffd\\u04cf\\u07fd\177\\ufffd\\ud82f\\udff7\\ufffd\\u02fb\\u98bd\\udfdf\\ufffd\\u0672\\u02d7\\u5c6f*"
            "\177\\uf5fb\\u0007\\u07ce\\u000f\\u581c\\udec7\\u000f^="
            "\\u06f8\\u18af\\udff7\\u077d\\ufffd\\ufffd\\u98aa\\ude5f\\ufffd\\u583b\\udd7c\\ufffd\\u98a7\\ude5eW\\u07f9"
            "3y\\u06ffu\\u0775\\u01ff\\u05f9l\\ufcb5\\u057d\\u07eb\\u06df\\ufffd\\u076e\\u077eE?xs\\ufffd$_"
            "l\\u07fa\\ud877\\uded5\\u57ff\\udf3a\\u0162\\ufffd\\u03f6\\u00c8=\\u078b\\u00f7>3\177{"
            "\\u07d0\\u0007\\u0006k\\u0792\\u02faZ\\u03faT\\u582c\\udf6br{1\\u06ee\\u038fI\\b\\u05b7R\\ufffd{"
            "w\\u01fb\\u0012\\u001e\\u5461\\ufffd=Y\\u0014\\u0365@"
            "\\ud898\\uddc6\\ua91d\\u01b4\\ufffd\\ud67e\\u07a1\\u001ay\\u0589\\ufffd\\f\\uad2b?"
            "\\ufffd\\u07e6\\u97df\\uddf1\\u0004\\ufffd\\u00ac\\u58a1\\ude64eI\\u0334Q\\u6bc9lW\\u96da\\u0515v\\u0467?"
            "\\u04fe\\u0392y\\u075f\\u04e4\\u052d\\u02b9t\\u9878\\udda6\\r\\u068d/"
            "\\u02b6\\u079e\\u030b\\ua8e3\\u0627\\u03effGDp\\u0006N\\u075cU\\u0666f\\u0015\\u0171\\ufffd\\u0000v\\ufffd"
            "\\ufffd\\ufffdJ\\u0004\\u0000\\u0000\",\"code\":200,\"headers\":{\"Cache-Control\":\"private\",\"Content-"
            "Encoding\":\"gzip\",\"Content-Length\":\"793\",\"Content-Type\":\"text/html; "
            "charset=utf-8\",\"Date\":\"Sat, 11 Feb 2023 14:37:00 "
            "GMT\",\"Server\":\"Microsoft-IIS/"
            "10.0\",\"Set-Cookie\":\"ARRAffinity=88eb07764da9fb4879ac3c8215c73aa6e77bd078ebc0fe627e51d6db747a12ec;Path="
            "/;HttpOnly;Domain=anglesharp.azurewebsites.net\",\"Vary\":\"Accept-Encoding\",\"X-AspNet-Version\":\"4.0."
            "30319\",\"X-AspNetMvc-Version\":\"5.0\",\"X-Frame-Options\":\"SAMEORIGIN\",\"X-Powered-By\":\"ASP.NET\"}"
            "}"));
        APSARA_TEST_TRUE(UnitTestHelper::LogKeyMatched(&allData[3], "protocol", "http"));

        std::cout << allData[3].DebugString() << std::endl;
    }

    void TestHTTPParserGC() {
        NetworkObserver* mObserver = new NetworkObserver();
        INT64_FLAG(sls_observer_network_process_timeout) = 18446744073;
        INT64_FLAG(sls_observer_network_connection_timeout) = 18446744073;
        INT64_FLAG(sls_observer_network_process_no_connection_timeout) = 0;
        BOOL_FLAG(sls_observer_network_protocol_stat) = true;

        NetworkStatistic* networkStatistic = NetworkStatistic::GetInstance();
        ProtocolDebugStatistic* protocolDebugStatistic = ProtocolDebugStatistic::GetInstance();
        mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
        networkStatistic->Clear();
        APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);

        {
            std::vector<std::string> rawHexs{rawHex1};
            RawNetPacketReader reader("30.43.120.83", false, ProtocolType_HTTP, rawHexs);
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
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionNum, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionCachedSize, 1);

            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 2);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionNum, 0);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionCachedSize, 0);
        }
        networkStatistic->Clear();
        {
            std::vector<std::string> rawHexs{rawHex2};
            RawNetPacketReader reader("30.43.120.83", false, ProtocolType_HTTP, rawHexs);
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
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionNum, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionCachedSize, 0); // stores in buffer not in cache


            mObserver->GarbageCollection(GetCurrentTimeInNanoSeconds());
            APSARA_TEST_EQUAL(mObserver->mAllProcesses.size(), 0);
            APSARA_TEST_EQUAL(networkStatistic->mGCCount, 2);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseConnCount, 1);
            APSARA_TEST_EQUAL(networkStatistic->mGCReleaseProcessCount, 1);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionNum, 0);
            APSARA_TEST_EQUAL(protocolDebugStatistic->mHTTPConnectionCachedSize, 0);
        }
    }


    const std::string req1_0
        = "00000000040088665a1cadb408004500014200004000400640dc1ef06481cb77a9f1fe9e005037927c12d638e91650181000c0270000"
          "504f5354202f6120485454502f312e310d0a486f73743a206f63732d6f6e656167656e742d7365727665722e616c69626162612e636f"
          "6d0d0a436f6e74656e742d547970653a206170706c69636174696f6e2f6a736f6e0d0a436f6e6e656374696f6e3a206b6565702d616c"
          "6976650d0a4163636570743a202a2f2a0d0a557365722d4167656e743a20436c6f75645368656c6c2f382e352e3020284d6163204f53"
          "20582062616e2062656e31332e322862616e2068616f323244343929290d0a4163636570742d4c616e67756167653a207a682d48616e"
          "732d434e3b713d310d0a4163636570742d456e636f64696e673a20677a69702c206465666c6174650d0a436f6e74656e742d4c656e67"
          "74683a20313537350d0a0d0a";
    const std::string req1_1
        = "00000000040088665a1cadb40800450005c80000400040063c561ef06481cb77a9f1fe9e005037927d2cd638e9165010100088260000"
          "7b2276657273696f6e223a332c2276616c7565223a7b2263736964223a2238444239384539462d303237442d344542422d393246312d"
          "393234414234383544324230222c226b6579223a22644a61515c2f6a7a70326d2b424d385469786b6e536b4c5c2f67337a56472b587a"
          "4b773255444c43566b4c5545696573776535616f6f4b72535277316f634379647a334f5975796e62744f764d4853506234693252496d"
          "6e71746874384157394932543563383350354b5c2f5662573962744238335c2f6a4c71464e557466414a584b474f3577637a706c6334"
          "424e58616b547944645234507a77714d63476633667a556b70727a4b4439346d30733d222c2276616c7565223a22743062594938424a"
          "36764d4144793139427662616c77416d4b724f366f384b41316d544864774b784a543544646461614a5a302b4b69693151346f486e6e"
          "5072764a6a4a73545870316b5a4b50645730315c2f4e4679584b78396d457368747a586d35564b48576e6b6c53536667705967413569"
          "7773612b506668434a775676436f434e663272686b50713637435472674532637744466b4e6c66436c685066492b4d32375374554861"
          "6d45313964485766754e585349677271535a3559756d32675166734855752b647834476a45596254435153566d304c553977626e486c"
          "37544f67685745415862644f687848574a44384b2b33514c7632797952747636757942787052726b304b357542546e6a443172554d46"
          "71564a6856307a69734d43514b323339516e336746714e76564b57386c62395770307a42456268413639776345446c5a6b31794c6744"
          "73306d3942722b62366533536a43557062496964766766434562665a52347a37744a415659664e685656456374527047643330336442"
          "4e414a53306b6236326d6d366c6762724e717267344e76686d3345656733584d445a454147544e775458317733666e494e742b583732"
          "7769614a366c4a6650642b2b727451433458366a493936536c626e44574d4353444f396b653341767770536c6a424e33416531674449"
          "312b7645564233396742505c2f70395a626c2b664c326b5633706159685764554a754f6159364465424d704c584a5c2f6f6d694d745a"
          "6378444859772b43715934786e667a375a6b62386c7a5377795942625150325558616c4f684756576c635666547458736976476e794b"
          "515848387271704165444d4d306d6c5c2f7a4b36595377304553445757397475536c596c7452596832675c2f73786939556557365346"
          "57393138716275584a6238793643566c55737a6f7369447850594774582b52485a596d566c6d336b364e4875796954796b2b74637471"
          "785633384e6666316d6879394366686b706d647954614f777a636a684c776e5775372b50456e7835734f43516541666f437976327553"
          "7163624e7466556474326b41653670324e5171686d31747176534b6333364b7377713165397736777675774b4e764f6f757639303932"
          "31574152677450673562396446594d3641726e547a3675716339316d6a737272414b7135594f4f423767707349316550326c58544448"
          "30626b5a59517a6747614a564f53774f6f4a5536597265544c643148304c36795544746d42734c776f7342514262445862494d575946"
          "6d2b4743396e6f6a6545517772656158654f43693967486a446e4536706d7434707041496c43707a6747775034796e336e4575503851"
          "4e664644344c426a686e71485057736f536c744e3078366c6e6159706666696a334f6c7a5841654d7a437a725633324266324841754a"
          "56545373306a784d74514c5c2f6169394e576f5473715249624e796e65585a6275316168484b6d4d616862354b57435a6a6b63546842"
          "6c474c706a467943716f376568665935706a30377030697a4e6b456e692b73435646444136456a7334375739764567506b6a5063674d"
          "6567624151514c754d674772625636476337305376476b6f366b54506739496e666e7851";
    const std::string req1_2
        = "00000000040088665a1cadb40800450000af000040004006416f1ef06481cb77a9f1fe9e0050379282ccd638e916501810002c6f0000"
          "7867426b464d4c5a73656e616b6833727771334a4538324a61566e627537446c63367970724376476941547a5a634c6675716e664356"
          "392b636764504133556d622b32754f696f5a4777392b7438725269426d455945513d222c227273615f6b65795f76657273696f6e223a"
          "2233222c226165735f6b65795f76657273696f6e223a2233227d7d";


    const std::string resp1_0
        = "88665a1cadb400000000040008004500014c59e540003306f3eccb77a9f11ef064810050fe9ed638e916379283535018000e316e0000"
          "485454502f312e3120323030200d0a446174653a204672692c2031302046656220323032332030323a33333a343120474d540d0a436f"
          "6e74656e742d547970653a206170706c69636174696f6e2f6a736f6e3b636861727365743d5554462d380d0a436f6e74656e742d4c65"
          "6e6774683a2035330d0a436f6e6e656374696f6e3a206b6565702d616c6976650d0a582d4170706c69636174696f6e2d436f6e746578"
          "743a206f63732d6f6e656167656e742d7365727665723a373030310d0a5365727665723a2054656e67696e652f417365727665720d0a"
          "4561676c654579652d547261636549643a203231353034366634313637353939363432313238393336323465373739390d0a54696d69"
          "6e672d416c6c6f772d4f726967696e3a202a0d0a0d0a";
    const std::string resp1_1
        = "88665a1cadb400000000040008004500005d59e640003306f4dacb77a9f11ef064810050fe9ed638ea3a379283535018000eec930000"
          "7b22656e63727970746564223a747275652c2276616c7565223a2274773276575035707a3533414c37614e7a48715565673d3d227d";

    const std::string rawHex1 = "00749c945d39a07817a0852e080045000071000040004006a0581e2b7853dcb526fbcd4700506b7401eca5"
                                "91a5ce5018100037ad0000474554202f20485454502f312e310d0a486f73743a2062616964752e636f6d0d"
                                "0a557365722d4167656e743a206375726c2f372e37372e300d0a4163636570743a202a2f2a0d0a0d0a";
    const std::string rawHex2
        = "a07817a0852e00749c945d390800450001598c7240002a0628fedcb526fb1e2b78530050cd47a591a5ce6b74023550180304c2650000"
          "485454502f312e3120323030204f4b0d0a446174653a205468752c203237204a616e20323032322030393a34363a313320474d540d0a"
          "5365727665723a204170616368650d0a4c6173742d4d6f6469666965643a205475652c203132204a616e20323031302031333a34383a"
          "303020474d540d0a455461673a202235312d34376366376536656538343030220d0a4163636570742d52616e6765733a206279746573"
          "0d0a436f6e74656e742d4c656e6774683a2038310d0a43616368652d436f6e74726f6c3a206d61782d6167653d38363430300d0a4578"
          "70697265733a204672692c203238204a616e20323032322030393a34363a313320474d540d0a436f6e6e656374696f6e3a204b656570"
          "2d416c6976650d0a436f6e74656e742d547970653a20746578742f68746d6c0d0a0d0a";


    const std::string chunk_req
        = "a41a3a6d057488665a1cadb40800450001fc000040004006f4c2c0a8002d28705af4ff0f0050f27263fe7b41f6df8018080448930000"
          "0101080a90ed09042f75e7cb474554202f4368756e6b656420485454502f312e310d0a486f73743a20616e676c6573686172702e617a"
          "75726577656273697465732e6e65740d0a436f6e6e656374696f6e3a206b6565702d616c6976650d0a557067726164652d496e736563"
          "7572652d52657175657374733a20310d0a557365722d4167656e743a204d6f7a696c6c612f352e3020284d6163696e746f73683b2049"
          "6e74656c204d6163204f5320582031305f31355f3729204170706c655765624b69742f3533372e333620284b48544d4c2c206c696b65"
          "204765636b6f29204368726f6d652f3131302e302e302e30205361666172692f3533372e33360d0a4163636570743a20746578742f68"
          "746d6c2c6170706c69636174696f6e2f7868746d6c2b786d6c2c6170706c69636174696f6e2f786d6c3b713d302e392c696d6167652f"
          "617669662c696d6167652f776562702c696d6167652f61706e672c2a2f2a3b713d302e382c6170706c69636174696f6e2f7369676e65"
          "642d65786368616e67653b763d62333b713d302e370d0a4163636570742d456e636f64696e673a20677a69702c206465666c6174650d"
          "0a4163636570742d4c616e67756167653a207a682c7a682d434e3b713d302e390d0a0d0a";
    const std::string chunk_resp_0
        = "88665a1cadb4a41a3a6d05740800450402bf324a4000680699b128705af4c0a8002d0050ff0f7b41f6dff27265c680180405c7f20000"
          "0101080a2f75f33690ed0904485454502f312e3120323030204f4b0d0a436f6e74656e742d547970653a20746578742f68746d6c0d0a"
          "446174653a205361742c2031312046656220323032332031343a33303a333520474d540d0a5365727665723a204d6963726f736f6674"
          "2d4949532f31302e300d0a43616368652d436f6e74726f6c3a20707269766174650d0a436f6e74656e742d456e636f64696e673a2067"
          "7a69700d0a5365742d436f6f6b69653a20415252416666696e6974793d38386562303737363464613966623438373961633363383231"
          "3563373361613665373762643037386562633066653632376535316436646237343761313265633b506174683d2f3b487474704f6e6c"
          "793b446f6d61696e3d616e676c6573686172702e617a75726577656273697465732e6e65740d0a5472616e736665722d456e636f6469"
          "6e673a206368756e6b65640d0a566172793a204163636570742d456e636f64696e670d0a582d4173704e65744d76632d56657273696f"
          "6e3a20352e300d0a582d4173704e65742d56657273696f6e3a20342e302e33303331390d0a582d506f77657265642d42793a20415350"
          "2e4e45540d0a0d0a63630d0a1f8b0800000000000400edbd07601c499625262f6dca7b7f4af54ad7e074a10880601324d8904010ecc1"
          "88cde692ec1d69472329ab2a81ca6556655d661640cced9dbcf7de7befbdf7de7befbdf7ba3b9d4e27f7dfff3f5c6664016cf6ce4ada"
          "c99e2180aac81f3f7e7c1f3f221effae4fbf3c79f3fbbc3c4de7eda23cfa8d93c7f89996d9f2e2b37cc97fe7d90c3f17799ba5d37956"
          "3779fbd9c7ebf67cfbe0637cde166d991f9dccd7cbb7f92c6deb6cd99ce7759a2fa7d5ac585ea46ddeb48fef4a2b6a7ed7c09b54b3eb"
          "0d0a380d0aa3c7f3dd1b5f9def0d0a";
    const std::string chunk_resp_1 = "88665a1cadb4a41a3a6d057408004504005e324b400068069c1128705af4c0a8002d0050ff0f7b41f"
                                     "96af27265c6801804057e1700000101080a2f75f39890ed090432340d0a1e3d9edf3f7a332f9a94fe"
                                     "0724e4853a6f56d5b2c9d3ecbca5d7767776d24533a617ee1f0d0a";
    const std::string chunk_resp_2
        = "88665a1cadb4a41a3a6d0574080045040085324c400068069be928705af4c0a8002d0050ff0f7b41f994f27265c680180405a94b0000"
          "0101080a2f75f79590ed14d733650d0addee85b4c9a7d572364edfcc73fabdbea40f9b79b52e67e9b26ad3695951db16dfb5759e2dd2"
          "497e5ed5f476590a48824d7f36f9b24ddb0afd9405fd2e180d0a380d0a3cbe2b03bc0b7a1e0d0a";
    const std::string chunk_resp_3
        = "88665a1cadb4a41a3a6d0574080045040043324d400068069c2a28705af4c0a8002d0050ff0f7b41f9e5f27265c680180405e4d20000"
          "0101080a2f75f7a590ed14d7610d0afd3f7a9bfb5d670100000d0a";
    const std::string chunk_resp_4 = "88665a1cadb4a41a3a6d0574080045040039324e400068069c3328705af4c0a8002d0050ff0f7b41f"
                                     "9f4f27265c68018040550a800000101080a2f75f7a590ed14d7300d0a0d0a";
    const std::string simple_post
        = "a41a3a6d057488665a1cadb40800450001fc000040004006f4c2c0a8002d28705af4ff0f0050f27263fe7b41f6df8018080448930000"
          "0101080a90ed09042f75e7cb504f5354202f506f7374416e797468696e6720485454502f312e310d0a486f73743a20616e676c657368"
          "6172702e617a75726577656273697465732e6e65740d0a436f6e6e656374696f6e3a206b6565702d616c6976650d0a436f6e74656e74"
          "2d4c656e6774683a203133350d0a43616368652d436f6e74726f6c3a206d61782d6167653d300d0a557067726164652d496e73656375"
          "72652d52657175657374733a20310d0a557365722d4167656e743a204d6f7a696c6c612f352e3020284d6163696e746f73683b20496e"
          "74656c204d6163204f5320582031305f31355f3729204170706c655765624b69742f3533372e333620284b48544d4c2c206c696b6520"
          "4765636b6f29204368726f6d652f3131302e302e302e30205361666172692f3533372e33360d0a4f726967696e3a20687474703a2f2f"
          "616e676c6573686172702e617a75726577656273697465732e6e65740d0a436f6e74656e742d547970653a206170706c69636174696f"
          "6e2f782d7777772d666f726d2d75726c656e636f6465640d0a4163636570743a20746578742f68746d6c2c6170706c69636174696f6e"
          "2f7868746d6c2b786d6c2c6170706c69636174696f6e2f786d6c3b713d302e392c696d6167652f617669662c696d6167652f77656270"
          "2c696d6167652f61706e672c2a2f2a3b713d302e382c6170706c69636174696f6e2f7369676e65642d65786368616e67653b763d6233"
          "3b713d302e370d0a526566657265723a20687474703a2f2f616e676c6573686172702e617a75726577656273697465732e6e65742f50"
          "6f7374416e797468696e670d0a4163636570742d456e636f64696e673a20677a69702c206465666c6174650d0a4163636570742d4c61"
          "6e67756167653a207a682c7a682d434e3b713d302e390d0a436f6f6b69653a205f5f52657175657374566572696669636174696f6e54"
          "6f6b656e3d433066565247717841555145686d523663624b456a4f74697465595350437a32777757746c6c693758704672485a347031"
          "4d306b3642553661415359314b646b5772663335646e5550544955375f466848397a685a4d564e6a53687935517a387a456d635f7054"
          "6d355845310d0a0d0a5f5f52657175657374566572696669636174696f6e546f6b656e3d2d39686c36564c4868534448583452353559"
          "58466e554e6c41716c5852313848653865526e39375246674e5857752d414c4b6a6a48326f6e7a59476632554a54626b655579617441"
          "4b666432765377354b484a4b3353717a546642656d2d52366b78702d51364d6d37535531";
    const std::string simple_post_resp
        = "88665a1cadb4a41a3a6d05740800450402bf324a4000680699b128705af4c0a8002d0050ff0f7b41f6dff27265c680180405c7f20000"
          "0101080a2f75f33690ed0904485454502f312e3120323030204f4b0d0a436f6e74656e742d4c656e6774683a203739330d0a436f6e74"
          "656e742d547970653a20746578742f68746d6c3b20636861727365743d7574662d380d0a446174653a205361742c2031312046656220"
          "323032332031343a33373a303020474d540d0a5365727665723a204d6963726f736f66742d4949532f31302e300d0a43616368652d43"
          "6f6e74726f6c3a20707269766174650d0a436f6e74656e742d456e636f64696e673a20677a69700d0a5365742d436f6f6b69653a2041"
          "5252416666696e6974793d38386562303737363464613966623438373961633363383231356337336161366537376264303738656263"
          "3066653632376535316436646237343761313265633b506174683d2f3b487474704f6e6c793b446f6d61696e3d616e676c6573686172"
          "702e617a75726577656273697465732e6e65740d0a566172793a204163636570742d456e636f64696e670d0a582d4173704e65744d76"
          "632d56657273696f6e3a20352e300d0a582d4672616d652d4f7074696f6e733a2053414d454f524947494e0d0a582d4173704e65742d"
          "56657273696f6e3a20342e302e33303331390d0a582d506f77657265642d42793a204153502e4e45540d0a0d0a1f8b08000000000004"
          "00edbd07601c499625262f6dca7b7f4af54ad7e074a10880601324d8904010ecc188cde692ec1d69472329ab2a81ca6556655d661640"
          "cced9dbcf7de7befbdf7de7befbdf7ba3b9d4e27f7dfff3f5c6664016cf6ce4adac99e2180aac81f3f7e7c1f3f221effae4fbf3c79f3"
          "fbbc3c4de7eda23cfa8d93c7f6679ecdf07391b7593a9d677593b79f7db46ecfb70f3e4aefe29bb668cbfce865d5b4c7cbeb765e2c2f"
          "1edf95cfe8cbbbe6fd4935bba69ff8eff17cb7d39c3ea08fcfab7a9166d3b6a8969f7d74d76ff1514addcfabd9671fade8d38f8e1e17"
          "cbd5ba4d97d922ffeca3dffff77f95ffa275deb43f99d7c57931cd00e04df5365f7e94b6d72b6a312f6633fc7599956bfa337bfdedbd"
          "9fbc683f6db7dbaf7ef259797df6eddd9dabc5b7bf5ceeec9dff607e555d2f7eefd5f4f7fafcec2716dffd89fcdef1eeeb6cf513273f"
          "f1532fbe583fbdf7536f663fa85e7cba9e7f79bcfb533ff1e017fd60fa8b5efce4c96afde2f5ab55fd55fd62f97bef7c7efda6ae7fff"
          "7cef271e3617bba0514acfe35971994ecbac693efb08e3dc9e5775f1836ad966e54734766ac1cfe3f9de91193491652ff8aa2658a01f"
          "fdc10fc32c882875deac4b228bfb0a0f7fad5d166dbee87e8fe771b3ca96a6d1dbfcfaa3a3616a3ebe8bc63701611a7f74b4fd705e7e"
          "fa93cfbf3d7ffdf4dbbff7feabfbf77f9fdffbd9f2ab17e5f12f2a7fef57bb07dfce0ff257cb870f5e3dbb78f17b7f77bd7dfcfcf7fa"
          "e99ffef65eb5fcc1eff3f9f9de57df7933799b7f759db5c7bfd7f96ceff2f5d5fddfebdbdff9bdeebdfe453f7873fe245f6cbffaf4ed"
          "bbd5f64f7cfac5e2c1ebaf76a3883dbe4bc3f73e337f7b9fd007066b9e928bba5aafba54f25b4dab727b31dbaececf4908b6f752fd7b"
          "77a7fb121ee551e1c0663d5914ade540f0f64706eaa45da6f4ffed597e9e611a799609807d0ceaf42b3ffedfe6f7c7773104fca26cf2"
          "b859646549cc7451e6af496c57e91b9ad4557691a73f933eced2799d9f9364b4edaa7974f76e86660d9a8d2f8a76be9e8c8beaa3a3d8"
          "a78fef66474470064ebddc55b926661585f1ff0076fcfcfe4a040000";
};

APSARA_UNIT_TEST_CASE(ProtocolHttpUnittest, TestCommonRequest, 0);
APSARA_UNIT_TEST_CASE(ProtocolHttpUnittest, TestCommonResponse, 0);
APSARA_UNIT_TEST_CASE(ProtocolHttpUnittest, TestHTTPPacketReaderOrder, 0);
APSARA_UNIT_TEST_CASE(ProtocolHttpUnittest, TestHTTPPacketReaderUnorder, 0);
APSARA_UNIT_TEST_CASE(ProtocolHttpUnittest, TestHTTPParserGC, 0);
APSARA_UNIT_TEST_CASE(ProtocolHttpUnittest, TestCommonRequest2, 0);
APSARA_UNIT_TEST_CASE(ProtocolHttpUnittest, TestHTTPPacketChunkedAndPost, 0);

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}