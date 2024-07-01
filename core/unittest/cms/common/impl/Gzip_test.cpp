#include <gtest/gtest.h>
#include "common/Gzip.h"
#include <iostream>

using namespace std;
using namespace common;

class CommonGZipTest : public testing::Test {
};

TEST_F(CommonGZipTest, test) {
    const string test = "helloworld";
    // char *buf = new char[test.size() * 3];
    // char *result = new char[test.size() * 3];
    // memset(result, 0, test.size() * 3);
    string testResult;
    int len = GZip::Compress(test, testResult);
    EXPECT_EQ(len, 0);
    // 检查gzip头部标识符
    EXPECT_EQ((unsigned char)testResult[0], (unsigned char)'\x1f');
    EXPECT_EQ((unsigned char)testResult[1], (unsigned char)'\x8b');
    cout << "size=" << testResult.size() << endl;
    std::string result;
    int deCompressResult = GZip::DeCompress(testResult, result, test.size() * 2);
    EXPECT_EQ(deCompressResult, 0);
    // cout << "len " << len << ":" << deCompressResult << endl;
    // string test1;
    // test1.assign(result, resultLen);
    cout << result << endl;
    EXPECT_EQ(result, test);
    // delete[]buf;
    // delete[]result;
}

TEST_F(CommonGZipTest, test1) {
    string test;
    for (int i = 0; i < 1000000; i++) {
        int a = 'z' - 'a';
        char c;
        if (i % 2 == 0) {
            c = static_cast<char>('a' + (i * i) % a);
        } else {
            c = static_cast<char>('A' + (i * i) % a);
        }
        test.append(1, c);
    }
    cout << "origin size=" << test.size() << endl;
    string result1, result2;
    EXPECT_EQ(0, GZip::Compress(test, result1));
    cout << "compress size=" << result1.size() << endl;
    EXPECT_EQ(0, GZip::DeCompress(result1, result2, test.size()));
    cout << "decompress size=" << result2.size() << endl;
    EXPECT_EQ(result2, test);
}
