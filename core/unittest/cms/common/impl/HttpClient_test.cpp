//
// Created by 韩呈杰 on 2023/3/27.
//
#include <gtest/gtest.h>
#include <iostream>

#include <curl/curl.h>
#include "common/HttpClient.h"
#include "common/Logger.h"

using namespace common;

TEST(HttpClientTest, OK) {
    EXPECT_EQ(static_cast<int>(CURLE_OK), static_cast<int>(HttpClient::Success));
}

TEST(HttpClientTest, genUrl) {
    const char *url = "http://localhost:7070/api/v5/getPack";
    std::map<std::string, std::string> params{
            {"hostname", "d22c02101.cloud.c03&amtest11"},
    };
    std::string fullUrl = common::genUrl(url, params);
    std::cout << "ORIG_URL: " << url << std::endl;
    std::cout << "FULL_URL: " << fullUrl << std::endl;
    EXPECT_EQ(fullUrl, "http://localhost:7070/api/v5/getPack?hostname=d22c02101.cloud.c03%26amtest11");
}

TEST(HttpClientTest, genUrl2) {
    const char *url = "http://localhost:7070/api/v5/getPack?";
    std::map<std::string, std::string> params{
            {"hostname", "d22c02101.cloud.c03&amtest11"},
    };
    std::string fullUrl = common::genUrl(url, params);
    std::cout << "ORIG_URL: " << url << std::endl;
    std::cout << "FULL_URL: " << fullUrl << std::endl;
    EXPECT_EQ(fullUrl, "http://localhost:7070/api/v5/getPack?hostname=d22c02101.cloud.c03%26amtest11");
}

TEST(HttpClientTest, genUrl3) {
    const char *url = "http://localhost:7070/api/v5/getPack????";
    std::map<std::string, std::string> params{
            {"hostname", "d22c02101.cloud.c03&amtest11"},
    };
    std::string fullUrl = common::genUrl(url, params);
    std::cout << "ORIG_URL: " << url << std::endl;
    std::cout << "FULL_URL: " << fullUrl << std::endl;
    EXPECT_EQ(fullUrl, "http://localhost:7070/api/v5/getPack?hostname=d22c02101.cloud.c03%26amtest11");
}

TEST(HttpClientTest, genUrl4) {
    const char *url = "http://localhost:7070?first=A";
    std::map<std::string, std::string> params{
            {"last", "B&C"},
    };
    std::string fullUrl = common::genUrl(url, params);
    std::cout << "ORIG_URL: " << url << std::endl;
    std::cout << "FULL_URL: " << fullUrl << std::endl;
    EXPECT_EQ(fullUrl, "http://localhost:7070?first=A&last=B%26C");
}

TEST(HttpClientTest, httpGetBaidu) {
    const char *host = "https://www.baidu.com";
    HttpResponse response;
    int code = HttpClient::HttpGet(host, std::map<std::string, std::string>{}, response);
    EXPECT_EQ(0, code);
    EXPECT_EQ(200, response.resCode);
    std::cout << "<" << host << "> Body[:128]: " << response.result.substr(0, 128) << std::endl;
    EXPECT_NE(response.result, "");
}

// 苹果下使用系统自带ssl，不支持『弱』cipher
#if !defined(__APPLE__) && !defined(__FreeBSD__)
TEST(HttpClientTest, httpGetBaiduWithCipher) {
    HttpRequest req;
    req.url = "https://www.baidu.com";
    HttpResponse response;
    int code = HttpClient::HttpCurlWithCipher(req, response, "RC4-SHA");
    EXPECT_EQ(0, code);
    EXPECT_EQ(200, response.resCode);
    std::cout << "<" << req.url << "> Body[:128]: " << response.result.substr(0, 128) << std::endl;
    EXPECT_NE(response.result, "");
}
#endif

TEST(HttpClientTest, httpPostAliyunNoBody) {
    const char* host = "https://www.aliyun.com"; // post会重定向，最终返回404
    HttpRequest req;
    req.url = host;
    // req.body = "1";
    HttpResponse response;
    int code = HttpClient::HttpPost(req, response);
    EXPECT_EQ(0, code);
    // EXPECT_EQ(200, response.resCode);
    std::cout << "<" << host << "> Body[:128]: " << response.result.substr(0, 128) << std::endl;
    // EXPECT_NE(response.result, "");
}

TEST(HttpClientTest, httpPostAliyunWithBody) {
    const char* host = "https://www.aliyun.com"; // post会重定向，最终返回404
    HttpRequest req;
    req.url = host;
    req.body = "test";
    HttpResponse response;
    int code = HttpClient::HttpPost(req, response);
    EXPECT_EQ(0, code);
    // EXPECT_EQ(200, response.resCode);
    std::cout << "<" << host << "> Body[:128]: " << response.result.substr(0, 128) << std::endl;
    // EXPECT_NE(response.result, "");
}

TEST(HttpClientTest, UrlParse1) {
    std::string host, path;
    EXPECT_EQ(true, HttpClient::UrlParse("https://curl.haxx.se/libcurl/c/parseurl.html", host, path));
    EXPECT_EQ(host, "curl.haxx.se");
    EXPECT_EQ(path, "/libcurl/c/parseurl.html");
}

TEST(HttpClientTest, UrlParse2) {
    std::string host, path;
    EXPECT_EQ(true, HttpClient::UrlParse("https://curl.haxx.se/", host, path));
    EXPECT_EQ(host, "curl.haxx.se");
    EXPECT_EQ(path, "/");
}

TEST(HttpClientTest, UrlParse3) {
    std::string host, path;
    EXPECT_TRUE(HttpClient::UrlParse("https://curl.haxx.se/libcurl/c/parseurl.html?test=test", host, path));
    EXPECT_EQ(host, "curl.haxx.se");
    EXPECT_EQ(path, "/libcurl/c/parseurl.html");
}

TEST(HttpClientTest, UrlParse4) {
    std::string host, path;
    EXPECT_EQ(true, HttpClient::UrlParse("https://curl.haxx", host, path));
    EXPECT_EQ(host, "curl.haxx");
    EXPECT_EQ(path, "");
}

TEST(HttpClientTest, UrlEnDecode) {
    const std::string param{"1 2&>3"};
    std::string encoded = HttpClient::UrlEncode(param);
    std::string decoded = HttpClient::UrlDecode(encoded);
    std::cout << param << " => " << encoded << " => " << decoded << std::endl;
    EXPECT_EQ(encoded, "1%202%26%3E3");
    EXPECT_EQ(decoded, param);
}
TEST(HttpClientTest, IsTimeout) {
    EXPECT_TRUE(HttpClient::IsTimeout(CURLE_OPERATION_TIMEDOUT));
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
TEST(HttpClientTest, PostWithWeakCipher)
{
    HttpRequest request{};
    request.url = "https://sn-ingestion-ssl.tj-core-edge-compute-share.aliyuncs.com";
    request.timeout = 5;

    HttpResponse response{};
    auto ret = HttpClient::HttpPostWithCipher(request, response, "RC4-SHA");
    if (ret != HttpClient::Success) {
        LogError("{}", HttpClient::StrError(ret));
    }
    EXPECT_EQ(ret, 0);
}
#endif

TEST(HttpClientTest, HttpMethodToString) {
    EXPECT_EQ(toString(HttpMethod::GET), "GET");
    EXPECT_EQ(toString(HttpMethod::POST), "POST");
    EXPECT_EQ(toString(HttpMethod::HEAD), "HEAD");
    EXPECT_EQ(toString(HttpMethod(-1)), "UNKNOWN(-1)");

    EXPECT_ANY_THROW(toString(HttpMethod(-1), true));
}