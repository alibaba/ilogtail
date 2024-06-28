#include "common/Rsa.h"
#include "common/Logger.h"
#include "common/Base64.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include "common/ScopeGuard.h"

using namespace std;
using namespace common;

const std::string Rsa::DEFAULT_PRIVATE_KEY = R"(
-----BEGIN RSA PRIVATE KEY-----
MIICXQIBAAKBgQD0bgBLViLJ7sbCmjlyiAkOWpTCWt4mbJCfI78kLcAgZsKnD3Vg
hMeTiW1r6MA+R9FoC9ZSxfLWWEzUsOR0H9gNwEEStjuVR6U8enNwW6b8mS7989Op
+Q4PxbPYdV4uoKLxGGKQeiwdIO9PMxeWXPGETCi1PPgyLZFp2+OVrIYTWwIDAQAB
AoGBALi091v2t0tJOMGNsaOu0MkcAhXsfLskhxT6+lHokKrrfGSp9dT+AaKn0xwc
QknOE5xAdbEPDLaU+1ouYjSua6Bee9Y3k69PlptkW+lqEah06KBa7uQp31U7astb
AHeWMzLwbP+2XUgcl4jNHJqayCRP2XQ4BGc5NWWt5Zic8D1hAkEA+ppWFU/u7uB6
DhxF//RgbtlUHP1dpLB++td7OOzdJ/lm4IN+ET8uIKnd/shoOmHKvyAKwrGUWu2b
2BQx8osZPwJBAPmxoUFngoR9fd04Du4yq1l+LvrMS5TMETkG4dPRmNA23Dl8B3RV
gRgNLF4m5zdzlptqckU3Qe001n78XpCaAuUCQAxvZB4inUSVNvlEReTxh2d4uUfG
+sKVT3e7AY5NkpvNMGGrpLHOZMeSJkXiQ+nBuIHLYT1P+oCYkccjGWdjp5kCQF0A
VkmgHjLu8uRkrtr1sHDC2Qi88yHW6EtPTumwVbSn2lrm4XfpKQ4mSfI/lztGKEB2
41z4eeu6FHJz2V0OoCkCQQDGC5FDCRlAGvZGDKmGhni3Wtp6q4H4FO/w/AeBRBCp
sGplqxvapjztdMN3ux6pnq1FM4+swGOcspDLxMEkzufB
-----END RSA PRIVATE KEY-----
)";

const std::string Rsa::DEFAULT_PUBLIC_KEY = R"(
-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQD0bgBLViLJ7sbCmjlyiAkOWpTC
Wt4mbJCfI78kLcAgZsKnD3VghMeTiW1r6MA+R9FoC9ZSxfLWWEzUsOR0H9gNwEES
tjuVR6U8enNwW6b8mS7989Op+Q4PxbPYdV4uoKLxGGKQeiwdIO9PMxeWXPGETCi1
PPgyLZFp2+OVrIYTWwIDAQAB
-----END PUBLIC KEY-----
)";

static RSA *createRsaFromPublicKey(const std::string &publicKey);

// int Rsa::encryptPrivateKeyToFile(const string& privateKey, const string& passwd, const string& privateKeyFile)
// {
//     RSA *pRsa = nullptr;
//     BIO *bp = BIO_new_mem_buf((void*)privateKey.data(), privateKey.size());
//     if (nullptr == bp)
//     {
//         LogError( "create BIO_new_mem_buf failed");
//         return -1;
//     }
//     if (nullptr == (pRsa = PEM_read_bio_RSAPrivateKey(bp, nullptr, nullptr, nullptr)))
//     {
//         LogWarn("read rsa private key failed");
//         BIO_free_all(bp);
//         return -1;
//     }
//     BIO_free_all(bp);
//
//     if (nullptr == (bp = BIO_new_file(privateKeyFile.c_str(), "w")))
//     {
//         LogError( "create encrypted private key file failed");
//         return -1;
//     }
//     //use des and passwd to encrypt private key
//     if (1 != PEM_write_bio_RSAPrivateKey(bp, pRsa, EVP_des_ede3_ofb(), (unsigned char*)passwd.data(), passwd.size(), nullptr, nullptr))
//     {
//         LogError( "write ciphertext rsa private key failed");
//         BIO_free_all(bp);
//         return -1;
//     }
//
//     BIO_free_all(bp);
//     return 0;
// }
//
// int Rsa::decryptPrivateKeyFromFile(const string& privateKeyFile, const string& passwd, string& privateKey)
// {
//     RSA *pRsa = nullptr;
//     BIO *bp = BIO_new(BIO_s_file());
//     if (nullptr == bp)
//     {
//         LogError("new BIO file failed");
//         return -1;
//     }
//     if (1 != BIO_read_filename(bp, privateKeyFile.c_str()))
//     {
//         LogWarn("BIO read privateKeyFile({}) failed", privateKeyFile.c_str());
//         BIO_free_all(bp);
//         return -1;
//     }
//
//     OpenSSL_add_all_algorithms();
//     if (nullptr == (pRsa = PEM_read_bio_RSAPrivateKey(bp, &pRsa, nullptr, (void*)passwd.data())))
//     {
//         LogWarn("BIO read or decrypt PrivateKey failed");
//         BIO_free_all(bp);
//         return -1;
//     }
//     BIO_free_all(bp);
//
//     bp = BIO_new(BIO_s_mem());
//     if (nullptr == bp)
//     {
//         LogError("new BIO mem failed");
//         return -1;
//     }
//     if (1 != PEM_write_bio_RSAPrivateKey(bp, pRsa, nullptr, nullptr, 0, nullptr, nullptr))
//     {
//         LogError("write rsa private key failed");
//         BIO_free_all(bp);
//         return -1;
//     }
//
//     BUF_MEM *bptr = nullptr;
//     BIO_get_mem_ptr(bp, &bptr);
//     privateKey.append(bptr->data, strlen(bptr->data));
//     BIO_free_all(bp);
//     return 0;
// }

int Rsa::privateDecrypt(const string& source, const string& privateKey, string& deString)
{
    const int RsaPaddingMode = RSA_PKCS1_PADDING;
    int   iRet    = -1;
    RSA  *pRsa    = nullptr;

    if (source.empty())
    {
        LogWarn("source string is empty");
        return -1;
    }

    {
        BIO *bp = BIO_new_mem_buf((void *) privateKey.data(), static_cast<int>(privateKey.size()));
        if (nullptr == bp) {
            LogError("create BIO_new_mem_buf failed");
            return -1;
        }
        defer(BIO_free_all(bp));

        if (nullptr == (pRsa = PEM_read_bio_RSAPrivateKey(bp, nullptr, nullptr, nullptr))) {
            LogError("read rsa private key failed");
            // BIO_free_all(bp);
            return -1;
        }
        // BIO_free_all(bp);
    }

    int rsaLen = RSA_size(pRsa);
    defer(if (pRsa) RSA_free(pRsa));
    //size of source should be integral multiple of rsaLen
    if (0 != source.size() % rsaLen)
    {
        LogError("ciphertext length error");
        // RSA_free(pRsa);
        return -1;
    }


    int      deLen = 0;
    unsigned enLen = 0;
    unsigned srcReadLen = rsaLen;

    if (RsaPaddingMode == RSA_PKCS1_PADDING) {
        deLen = rsaLen - 11;
    }
    // else if (RsaPaddingMode == RSA_NO_PADDING)
    //     deLen = rsaLen;
    // else
    // {
    //     LogError("padding mode error");
    //     // RSA_free(pRsa);
    //     return -1;
    // }

    char *pDeBuf = (char *)malloc(deLen + 1);
    defer(if (pDeBuf) free(pDeBuf));
    memset(pDeBuf, 0, deLen + 1);

    //read peer segment
    deString.clear();
    auto *pSrc = (unsigned char*)source.data();
    while (enLen + srcReadLen <= source.size())
    {
        //ex:use RSA_PKCS1_PADDING, read rsaLen bytes ciphertext and output rsaLen-11 bytes plaintext segment
        iRet = RSA_private_decrypt(srcReadLen, pSrc, (unsigned char*)pDeBuf, pRsa, RsaPaddingMode);
        if (iRet < 0)
        {
            LogError("multi segment private decrypt failed");
            break;
        }
		deString.append(pDeBuf, iRet);
        memset(pDeBuf, 0, deLen + 1);
        pSrc += srcReadLen;
        enLen += srcReadLen;
    }

    // RSA_free(pRsa);
    // free(pDeBuf);

    if (iRet < 0)
    {
        LogError("PrivateKey decrypt failed");
        return -1;
    }
    return 0;
}

// int Rsa::pubEncryptByKeyFile(const string& source, const string& publicKeyFile, string& enString, int RsaPaddingMode)
// {
//     int   iRet    = -1;
//     int   rsaLen  = 0;
//     char *pEnBuf  = nullptr;
//     RSA  *pRsa    = nullptr;
//     FILE *pFile   = nullptr;
//
//     if (source.empty())
//     {
//         LogError("source string is empty");
//         return -1;
//     }
//
//     if (nullptr == (pFile = fopen(publicKeyFile.c_str(), "r")))
//     {
//         LogError("open pub key failed");
//         return -1;
//     }
//     {
//         defer(fclose(pFile));
//         if (nullptr == (pRsa = PEM_read_RSA_PUBKEY(pFile, nullptr, nullptr, nullptr))) {
//             LogError("read pub key failed");
//             // fclose(pFile);
//             return -1;
//         }
//     }
//
//     rsaLen = RSA_size(pRsa);
//     defer(if(pRsa) RSA_free(pRsa));
//     unsigned enLen = 0;
//     unsigned srcReadLen = 0;
//
//     if (RsaPaddingMode == RSA_PKCS1_PADDING)
//         srcReadLen = rsaLen - 11;
//     else if (RsaPaddingMode == RSA_NO_PADDING)
//         srcReadLen = rsaLen;
//     else
//     {
//         LogError("padding mode error");
//         // RSA_free(pRsa);
//         return -1;
//     }
//
//     pEnBuf = (char *)malloc(rsaLen + 1);
//     memset(pEnBuf, 0, rsaLen + 1);
//     defer(free(pEnBuf));
//
//     //read peer segment
//     enString.clear();
//     unsigned char *pSrc = (unsigned char*)source.c_str();
// 	while (enLen + srcReadLen <= source.size())
//     {
//         //ex: use RSA_PKCS1_PADDING, read srcReadLen bytes plaintext and output rsaLen bytes ciphertext segment
//         iRet = RSA_public_encrypt(srcReadLen, pSrc, (unsigned char*)pEnBuf, pRsa, RsaPaddingMode);
//         if (iRet < 0)
//         {
//             LogError("multi segment public encrypt failed");
//             // RSA_free(pRsa);
//             // free(pEnBuf);
//             return -1;
//         }
//
//         //must use rsaLen when append here!!!
//         enString.append(pEnBuf, iRet);
//         memset(pEnBuf, 0, rsaLen + 1);
//         pSrc += srcReadLen;
//         enLen += srcReadLen;
//     }
//
//     //read only one segment less than srcReadLen or segment left above
// 	if (enLen < source.size())
//     {
//         srcReadLen = source.size() % srcReadLen;
//         iRet = RSA_public_encrypt(srcReadLen, pSrc, (unsigned char*)pEnBuf, pRsa, RsaPaddingMode);
//         if (iRet < 0)
//         {
//             LogError("single segment public encrypt failed");
//         }
//         else
//         {
//             //must use rsaLen when append here!!!
//             enString.append(pEnBuf, iRet);
//         }
//     }
//
//     // RSA_free(pRsa);
//     // free(pEnBuf);
//
//     if (iRet < 0)
//     {
//         LogError("PublicKey encrypt failed");
//         return -1;
//     }
//     return 0;
// }
//
// bool Rsa::generateKey(std::string& privateKey, std::string& publicKey)
// {
//     int ret = 0;
//     RSA *r = nullptr;
//     BIGNUM *bne = nullptr;
//     BIO *bp_public = nullptr, *bp_private = nullptr;
//     int bits = 1024;
//     unsigned long e = RSA_F4;
//     int priKeyLen = 0, pubKeyLen = 0;
//     char *pemPublicKey = nullptr, *pemPrivateKey = nullptr;
//
//     // 1. generate rsa key
//     bne = BN_new();
//     ret = BN_set_word(bne,e);
//     if(ret != 1){
//         goto free_all;
//     }
//     r = RSA_new();
//     ret = RSA_generate_key_ex(r, bits, bne, nullptr);
//     if(ret != 1){
//         goto free_all;
//     }
//
//     // 2. save public key
//     // 在openssl中公钥的输出方式有两种（ int PEM_write_bio_RSA_PUBKEY(BIO *bp, RSA *x); 和 int PEM_write_bio_RSAPublicKey(BIO *bp, RSA *x);）
//     // 前者使用的SubjectPublicKeyInfo结构标准对RSA公钥进行编码操作，如果公钥类型不是RSA，就出错返回失败信息。
//     // 后者使用的是PKCS#1 RSAPublicKey结构标准对RSA公钥进行编码操作。
//     // 读写公钥的时候切记一定要配套使用，否则读出来返回的RSA结构体会是空的。
//     // 通过命令行生成的：openssl genrsa -out private.pem 2048; openssl rsa -in private.pem -outform PEM -pubout -out public.pem, 使用的是PUBKEY版本的
//     // java 代码中认识的也是PUBKEY版本的公钥，所以，放弃抵抗吧
//     bp_public = BIO_new(BIO_s_mem());
//     ret = PEM_write_bio_RSAPublicKey(bp_public, r);
//     if(ret != 1){
//         goto free_all;
//     }
//     pubKeyLen = BIO_pending(bp_public);
//     pemPublicKey = (char *)calloc(pubKeyLen + 1, 1); // \0 terminate string
//     BIO_read(bp_public, pemPublicKey, pubKeyLen);
//     publicKey = pemPublicKey;
//     free(pemPublicKey);
//
//     // 3. save private key
//     bp_private = BIO_new(BIO_s_mem());
//     ret = PEM_write_bio_RSAPrivateKey(bp_private, r, nullptr, nullptr, 0, nullptr, nullptr);
//     if (ret != 1) {
//         goto free_all;
//     }
//     priKeyLen = BIO_pending(bp_private);
//     pemPrivateKey = (char *)calloc(priKeyLen + 1, 1);
//     BIO_read(bp_private, pemPrivateKey, priKeyLen);
//     privateKey = pemPrivateKey;
//     free(pemPrivateKey);
//
//     // 4. free, release resources
// free_all:
//     BIO_free_all(bp_public);
//     BIO_free_all(bp_private);
//     RSA_free(r);
//     BN_free(bne);
//
//     return (ret == 1);
// }

int Rsa::pubEncryptByKey(const std::string& source, const std::string& publicKey,  std::string& enString)
{
    const int RsaPaddingMode = RSA_PKCS1_PADDING;
    int   iRet    = -1;
    // int   rsaLen  = 0;
    // char *pEnBuf  = nullptr;

    if (source.empty())
    {
        LogError("source string is empty");
        return -1;
    }

    RSA *pRsa = createRsaFromPublicKey(publicKey);
    if (pRsa == nullptr) {
        LogError("build RSA struct error");
        return -1;
    }
    defer(RSA_free(pRsa));

    int rsaLen = RSA_size(pRsa);
    size_t srcReadLen = 0;

    if (RsaPaddingMode == RSA_PKCS1_PADDING) {
        srcReadLen = rsaLen - 11;
    }
    // else if (RsaPaddingMode == RSA_NO_PADDING)
    //     srcReadLen = rsaLen;
    // else
    // {
    //     LogError("padding mode error");
    //     // RSA_free(pRsa);
    //     return -1;
    // }

    char *pEnBuf = (char *)malloc(rsaLen + 1);
    memset(pEnBuf, 0, rsaLen + 1);
    defer(free(pEnBuf));

    //read peer segment
    enString.clear();
    auto *pSrc = (unsigned char*)source.c_str();
    size_t enLen = 0;
	while (enLen + srcReadLen <= source.size())
    {
        //ex: use RSA_PKCS1_PADDING, read srcReadLen bytes plaintext and output rsaLen bytes ciphertext segment
        iRet = RSA_public_encrypt(srcReadLen, pSrc, (unsigned char*)pEnBuf, pRsa, RsaPaddingMode);
        if (iRet < 0)
        {
            LogError("multi segment public encrypt failed");
            // RSA_free(pRsa);
            // free(pEnBuf);
            return -1;
        }

        //must use rsaLen when append here!!!
        enString.append(pEnBuf, iRet);
        memset(pEnBuf, 0, rsaLen + 1);
        pSrc += srcReadLen;
        enLen += srcReadLen;
    }

    //read only one segment less than srcReadLen or segment left above
	if (enLen < source.size())
    {
        srcReadLen = static_cast<int>(source.size()) % srcReadLen;
        iRet = RSA_public_encrypt(srcReadLen, pSrc, (unsigned char*)pEnBuf, pRsa, RsaPaddingMode);
        if (iRet < 0)
        {
            LogError("single segment public encrypt failed");
        }
        else
        {
            //must use rsaLen when append here!!!
            enString.append(pEnBuf, iRet);
        }
    }

    // RSA_free(pRsa);
    // free(pEnBuf);

    if (iRet < 0)
    {
        LogError("PublicKey encrypt failed");
        return -1;
    }
    return 0;
}

// int Rsa::priDecryptByKey(const std::string& source, const std::string& privateKey, std::string& deString, int RsaPaddingMode)
// {
//     return 0;
// }

static RSA *createRsaFromPublicKey(const std::string &publicKey) {
    const struct {
        decltype(&PEM_read_bio_RSA_PUBKEY) NewRsa;
        const char *Name;
    } fnNewRsaArray[] = {
            {&PEM_read_bio_RSA_PUBKEY,   "PEM_read_bio_RSA_PUBKEY"},
            {&PEM_read_bio_RSAPublicKey, "PEM_read_bio_RSAPublicKey"},
    };
    const size_t count = sizeof(fnNewRsaArray)/sizeof(fnNewRsaArray[0]);

    RSA *pRsa = nullptr;
    for (size_t i = 0; i < count && pRsa == nullptr; i++) {
        BIO *bp = BIO_new_mem_buf((void *) publicKey.data(), static_cast<int>(publicKey.size()));
        if (nullptr == bp) {
            LogError("create BIO_new_mem_buf failed");
            return nullptr;
        }
        defer(BIO_free_all(bp));

        pRsa = fnNewRsaArray[i].NewRsa(bp, nullptr, nullptr, nullptr);
        if (pRsa == nullptr) {
            // 如果常规版本标准的公钥不行，那么使用另一个标准的尝试
            LogWarn("{} fail{}", fnNewRsaArray[i].Name, (i + 1 < count ? ", try another style" : ""));
        }
    }

    return pRsa;
}

