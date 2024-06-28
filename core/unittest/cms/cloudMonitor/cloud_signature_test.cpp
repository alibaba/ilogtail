#include <gtest/gtest.h>
#include "cloudMonitor/cloud_signature.h"
#include <iostream>

using namespace std;
using namespace cloudMonitor;

class Cms_CloudSignatureTest : public testing::Test {
};

TEST_F(Cms_CloudSignatureTest, Calculate) {
    string data =
            R"XXX({"systemInfo":{"serialNumber":"17bd5a3b-62af-5a0e-b3e7-fe8ead2c67c0","hostname":"ali-186590d956fb.local","localIPs":["fe80::1","fe80::1822:85f2:89a7:2935","30.27.112.62","fe80::c7:e4ff:fee1:9bbf","fe80::80db:10c9:9c93:87b1"],"name":"Mac OS (darwin)","version":"10.13.5","arch":"amd64","freeSpace":60010225664},"versionInfo":{"version":"2.1.1"}})XXX";
    string accessSecret = "SRDzEi8yE_YPRZH8dVG-sg";
    string result;
    CloudSignature::Calculate(data, accessSecret, result);
    cout << "result=" << result << endl;
    const string sig = "QVQiF2TedtORjwk1ePijHsKDUdB8BjJIUvTqKUMd6RvBpH9Jo3c4pcdvSg7iUwVS";
    EXPECT_EQ(sig, result);
}

TEST_F(Cms_CloudSignatureTest, CalculateMetric1) {
    string signString = "POST\n"
                        "0B9BE351E56C90FED853B32524253E8B\n"
                        "application/json\n"
                        "Tue, 11 Dec 2018 21:05:51 +0800\n"
                        "x-cms-api-version:1.0\n"
                        "x-cms-ip:127.0.0.1\n"
                        "x-cms-signature:hmac-sha1\n"
                        "/metric/custom/upload";
    string accessSecret = "testsecret";
    string result;
    CloudSignature::CalculateMetric(signString, accessSecret, result);
    const string sig = "1DC19ED63F755ACDE203614C8A1157EB1097E922";
    EXPECT_EQ(sig, result);
}

TEST_F(Cms_CloudSignatureTest, CalculateMetric2) {
    string signString = "POST\n"
                        "c9f165a6811a00647eb10f50f4bc314d\n"
                        "text/plain\n"
                        "Tue, 13 Oct 2020 16:50:55 GMT\n"
                        "x-cms-agent-instance:host-abcdef1234\n"
                        "x-cms-agent-version:3.4.6\n"
                        "x-cms-api-version:1.1\n"
                        "x-cms-host:staragent-fenghua-coding\n"
                        "x-cms-ip:10.137.71.4\n"
                        "x-cms-signature:hmac-sha1\n"
                        "/metric/v2/put/testNamespace";
    string accessSecret = "5EB63746049CBB568BC0DBD56F453799";
    string result;
    CloudSignature::CalculateMetric(signString, accessSecret, result);
    const string sig = "FC30FFFE4F5A52BEF4BABB06D6D7E43462F16141";
    EXPECT_EQ(sig, result);
}

TEST_F(Cms_CloudSignatureTest, AesEncryptWithInvalidKey) {
    std::string key(25, '-');
    std::string result;
    EXPECT_EQ(-1, CloudSignature::AesEncrypted("", key, result));
}
