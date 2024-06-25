#ifndef _BASE64_H_
#define _BASE64_H_

#include <string>  
 
namespace common
{

class Base64
{
public:
    static std::string encode(const unsigned char* bytes_to_encode, size_t in_len);
    static std::string decode(const std::string& encoded_string);
    static std::string UrlEncode(const unsigned char* bytes_to_encode, size_t in_len);
    static std::string UrlDecode(const std::string& encoded_string);

};

}

#endif
