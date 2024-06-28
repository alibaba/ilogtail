#ifndef ARGUS_COMMON_GZIP_H
#define ARGUS_COMMON_GZIP_H
#include <string>
namespace common
{
class GZip
{
public:
    // static int Compress(const char *src, size_t srcLen, char *dest, size_t destLen);
    // static int DeCompress(const char *src, size_t srcLen, const char *dst, size_t dstLen);
    //如果非0，则需要设置count,默认压缩后大小不超过src.size()+1024;
    // static int Compress(const std::string &src,std::string &dest,size_t count=1024);
    //如果返回非0，则需要设置count,默认解压后大小不超过src.size()+1024;
    static int DeCompress(const std::string &src,std::string &dest,size_t count=1024);

    // 成功则返回 0， 否则为非 0
    static int Compress(const std::string &src, std::string &dest);
    
};
}
#endif // ! ARGUS_COMMON_GZIP_H
