#include "common/Base64.h"

using namespace common;

static const auto &base64_chars = *new std::string(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/");

static const auto &url_base64_chars = *new std::string(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_");

static inline bool is_base64(unsigned char c)
{  
    return (isalnum(c) || (c == '+') || (c == '/'));  
}  

static inline bool is_url_base64(unsigned char c)
{  
    return (isalnum(c) || (c == '-') || (c == '_'));  
}  

std::string Base64::encode(const unsigned char* bytes_to_encode, size_t in_len)
{  
    std::string ret;  
    int i = 0;  
    int j = 0;  
    unsigned char char_array_3[3];  
    unsigned char char_array_4[4];  

    while (in_len--)
    {  
        char_array_3[i++] = *(bytes_to_encode++);  
        if (i == 3)
        {  
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;  
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);  
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);  
            char_array_4[3] = char_array_3[2] & 0x3f;  

            for(i = 0; (i <4) ; i++)  
                ret += base64_chars[char_array_4[i]];  
            i = 0;  
        }  
    }  

    if (i)  
    {  
        for(j = i; j < 3; j++)  
            char_array_3[j] = '\0';  

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;  
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);  
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);  
        char_array_4[3] = char_array_3[2] & 0x3f;  

        for (j = 0; (j < i + 1); j++)  
            ret += base64_chars[char_array_4[j]];  

        while((i++ < 3))  
            ret += '=';  

    }  

    return ret;
}  

std::string Base64::decode(const std::string& encoded_string)
{  
    size_t in_len = encoded_string.size();  
    int i = 0;  
    int j = 0;  
    int in_ = 0;  
    unsigned char char_array_4[4], char_array_3[3];  
    std::string ret;  

    while (in_len--)
    {  
        if (encoded_string[in_] == '\r' || encoded_string[in_] == '\n')
        {
            in_++;
            continue;
        }

        if (encoded_string[in_] == '=' || !is_base64(encoded_string[in_]))
            break;

        char_array_4[i++] = encoded_string[in_]; in_++;  
        if (i ==4)
        {  
            for (i = 0; i <4; i++)  
                char_array_4[i] = base64_chars.find(char_array_4[i]);  

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);  
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);  
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];  

            for (i = 0; (i < 3); i++)  
                ret += char_array_3[i];  
            i = 0;  
        }
    }  

    if (i)
    {  
        for (j = i; j <4; j++)  
            char_array_4[j] = 0;  

        for (j = 0; j <4; j++)  
            char_array_4[j] = base64_chars.find(char_array_4[j]);  

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);  
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);  
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];  

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];  
    }  

    return ret;  
}

std::string Base64::UrlEncode(const unsigned char* bytes_to_encode, size_t in_len)
{  
    std::string ret;  
    int i = 0;  
    int j = 0;  
    unsigned char char_array_3[3];  
    unsigned char char_array_4[4];  

    while (in_len--)
    {  
        char_array_3[i++] = *(bytes_to_encode++);  
        if (i == 3)
        {  
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;  
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);  
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);  
            char_array_4[3] = char_array_3[2] & 0x3f;  

            for(i = 0; (i <4) ; i++)  
                ret += url_base64_chars[char_array_4[i]];  
            i = 0;  
        }  
    }  

    if (i)  
    {  
        for(j = i; j < 3; j++)  
            char_array_3[j] = '\0';  

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;  
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);  
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);  
        char_array_4[3] = char_array_3[2] & 0x3f;  

        for (j = 0; (j < i + 1); j++)  
            ret += url_base64_chars[char_array_4[j]];  

        while((i++ < 3))  
            ret += '=';  

    }  

    return ret;
}  

std::string Base64::UrlDecode(const std::string& encoded_string)
{  
    size_t in_len = encoded_string.size();  
    int i = 0;  
    int j = 0;  
    int in_ = 0;  
    unsigned char char_array_4[4], char_array_3[3];  
    std::string ret;  

    while (in_len--)
    {  
        if (encoded_string[in_] == '\r' || encoded_string[in_] == '\n')
        {
            in_++;
            continue;
        }

        if (encoded_string[in_] == '=' || !is_url_base64(encoded_string[in_]))
            break;

        char_array_4[i++] = encoded_string[in_]; in_++;  
        if (i ==4)
        {  
            for (i = 0; i <4; i++)  
                char_array_4[i] = url_base64_chars.find(char_array_4[i]);  

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);  
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);  
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];  

            for (i = 0; (i < 3); i++)  
                ret += char_array_3[i];  
            i = 0;  
        }
    }  

    if (i)
    {  
        for (j = i; j <4; j++)  
            char_array_4[j] = 0;  

        for (j = 0; j <4; j++)  
            char_array_4[j] = url_base64_chars.find(char_array_4[j]);  

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);  
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);  
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];  

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];  
    }  

    return ret;  
}