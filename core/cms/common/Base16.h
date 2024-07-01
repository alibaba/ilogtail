#ifndef BASE_16_H
#define BASE_16_H
#include <cstdlib>
#include <string>
namespace common
{
class Base16
{
public:
    static std::string Encode(const unsigned char *bytes, size_t bytesLength, bool isToUpper=true);
    static std::string EncodeToUpper(unsigned char *bytes, size_t bytesLength);
    static std::string EncodeToLower(unsigned char *bytes, size_t bytesLength);
    static std::string Decode(const char *hex);
private:
    static int hex2dec(char c);
};
}
#endif