#include "common/Base16.h"
#include <cstring>
namespace common
{
std::string Base16::EncodeToUpper(unsigned char* bytes, size_t bytesLength) {
    return Encode(bytes, bytesLength, true);
}

std::string Base16::EncodeToLower(unsigned char* bytes, size_t bytesLength) {
    return Encode(bytes, bytesLength, false);
}

std::string Base16::Encode(const unsigned char* bytes, size_t bytesLength, bool isToUpper) {
    const char *table = isToUpper ? "0123456789ABCDEF" : "0123456789abcdef";
    unsigned char highByte, lowByte;
    char *output = (char *)calloc((bytesLength << 1)+1, sizeof(char));
    for (size_t i = 0; i < bytesLength; i++) {
        //向右移动4bit，获得高4bit
        highByte = bytes[i] >> 4;
        //与0x0f做位与运算，获得低4bit
        lowByte = bytes[i] & 0x0F;

        //由于高4bit和低4bit都只有4个bit，他们转换成10进制的数字，范围都在0 ～ 15闭区间内
        //大端模式
        size_t j = i << 1;
        output[j] = table[highByte];
        output[j + 1] = table[lowByte];
    }
    std::string result = std::string(output);
    free(output);
    return result ;
}
//把16进制字符转换成10进制表示的数字
//通过man ascii命令查看ASCII编码表即可得到如下转换逻辑
int Base16::hex2dec(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    } else if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    } else {
        return 0;
    }
}

//input指向的字符串长度必须是2的整数倍
std::string Base16::Decode(const char* hex) {
    size_t inputLength = strlen(hex);
    size_t halfInputLength = inputLength >> 1;
    auto *output = (unsigned char *)calloc(halfInputLength+1, sizeof(unsigned char));
    for (size_t i = 0; i < halfInputLength; i++) {
        //16进制数字转换为10进制数字的过程
        size_t j = i << 1;
        output[i] = (hex2dec(hex[j]) << 4) + hex2dec(hex[j + 1]);
    }
    std::string result = std::string((const char *)output);
    free(output);
    return result ;
}
}