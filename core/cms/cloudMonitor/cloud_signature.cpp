#include "cloud_signature.h"
#include "openssl/hmac.h"
#include "openssl/aes.h"

#include "common/Base64.h"
#include "common/Logger.h"
#include "common/Base16.h"

using namespace std;
using namespace common;

namespace cloudMonitor
{
void CloudSignature::Calculate(const string &content, const string &secret, string &result)
{
    //先base64解码
    string password = Base64::UrlDecode(secret);
    unsigned char shaBuf[EVP_MAX_MD_SIZE] = {'\0'};
    unsigned int shaBufLen = 0;
    //hmac计算散列消息鉴别码
    HMAC(EVP_sha1(),
         password.c_str(), static_cast<int>(password.size()),
         (unsigned char *) content.c_str(), content.size(),
         shaBuf, &shaBufLen);
    string aesContent = "hello world:";
    aesContent.append((const char *) shaBuf, shaBufLen);
    string aesResult;
    if (AesEncrypted(aesContent, password, aesResult) < 0)
    {
        result = "";
    }
    else
    {
        result = Base64::UrlEncode((const unsigned char *) aesResult.data(), aesResult.size());
    }
}
void CloudSignature::CalculateMetric(const string &content, const string &secret, string &result)
{
    unsigned char shaBuf[EVP_MAX_MD_SIZE] = {'\0'};
    unsigned int shaBufLen = 0;
    //hmac计算散列消息鉴别码
    HMAC(EVP_sha1(),
         secret.c_str(),
         static_cast<int>(secret.size()),
         (unsigned char *) content.c_str(),
         content.size(),
         shaBuf,
         &shaBufLen);
    result = Base16::Encode(shaBuf, shaBufLen);
}

int CloudSignature::AesEncrypted(const string &src, const string &key, string &result)
{
    int padding = AES_BLOCK_SIZE - src.size() % AES_BLOCK_SIZE;
    size_t bufLen = padding + src.size();
    unique_ptr<unsigned char[]> pInput(new unsigned char[bufLen]);
    memcpy(pInput.get(), src.data(), src.size());
    memset(pInput.get() + src.size(), padding, bufLen - src.size()); // hancj.这个set成padding就有点儿奇怪
    if (key.size() != 16 && key.size() != 24 && key.size() != 32)
    {
        LogWarn("the keyLen({}) is invalid", key.size());
        return -1;
    }

    AES_KEY aes;
    if (AES_set_encrypt_key((const unsigned char *) key.data(), static_cast<int>(key.size() * 8), &aes) < 0)
    {
        LogWarn("Unable to set encryption key in AES");
        return -1;
    }
    unique_ptr<unsigned char[]> pResult(new unsigned char[bufLen]);
    auto *pIn = (unsigned char *) pInput.get();
    auto *pOut = (unsigned char *) pResult.get();
    for (size_t i = 0; i < bufLen / AES_BLOCK_SIZE; i++)
    {
        AES_ecb_encrypt(pIn + i * AES_BLOCK_SIZE, pOut + i * AES_BLOCK_SIZE, &aes, AES_ENCRYPT);
    }
    result.assign((const char *) pOut, bufLen);
    return static_cast<int>(bufLen);
}
}