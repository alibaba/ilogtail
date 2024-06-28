#ifndef ARGUS_COMMON__RSA_H
#define ARGUS_COMMON__RSA_H

#include <string>

namespace common
{

class Rsa
{
public :
    static const std::string DEFAULT_PRIVATE_KEY;

    static const std::string DEFAULT_PUBLIC_KEY;

public :
    // static int encryptPrivateKeyToFile(const std::string& privateKey, const std::string& passwd, const std::string& privateKeyFile);
    //
    // static int decryptPrivateKeyFromFile(const std::string& privateKeyFile, const std::string& passwd, std::string& privateKey);

    static int privateDecrypt(const std::string& source, const std::string& privateKey, std::string& deString);

    // static int pubEncryptByKeyFile(const std::string& source, const std::string& publicKeyFile, std::string& enString, int RsaPaddingMode = RSA_PKCS1_PADDING);

    // 生成密钥对
    // static bool generateKey(std::string& privateKey, std::string& publicKey);

    static int pubEncryptByKey(const std::string& source, const std::string& publicKey, std::string& enString);

    // static int priDecryptByKey(const std::string& source, const std::string& privateKey, std::string& deString, int RsaPaddingMode = RSA_PKCS1_PADDING);
};

}
#endif // !ARGUS_COMMON__RSA_H
