#include "common/Gzip.h"
// #ifdef WIN32
// #define ZLIB_WINAPI
// #endif
#include <zlib.h>
#include <vector>
#include <cstring> // std::memset
#include "common/ScopeGuard.h"

using namespace std;

namespace common {
// int GZip::Compress(const char *src, size_t srcLen, char *dest, size_t destLen)
// {
//     z_stream c_stream;
//     int err = 0;
//     int windowBits = 15;
//     int GZIP_ENCODING = 16;
//
//     if (src && srcLen > 0)
//     {
//         c_stream.zalloc = (alloc_func)0;
//         c_stream.zfree = (free_func)0;
//         c_stream.opaque = (voidpf)0;
//         if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//                          windowBits | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY) != Z_OK)
//             return -1;
//         c_stream.next_in = (Bytef *)src;
//         c_stream.avail_in = static_cast<decltype(c_stream.avail_in)>(srcLen);
//         c_stream.next_out = (Bytef *)dest;
//         c_stream.avail_out = static_cast<decltype(c_stream.avail_out)>(destLen);
//         while (c_stream.avail_in != 0 && c_stream.total_out < destLen)
//         {
//             if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
//                 return -1;
//         }
//         if (c_stream.avail_in != 0)
//             return c_stream.avail_in;
//         for (;;)
//         {
//             if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
//                 break;
//             if (err != Z_OK)
//                 return -1;
//         }
//         if (deflateEnd(&c_stream) != Z_OK)
//             return -1;
//         return c_stream.total_out;
//     }
//     return -1;
// }
// int GZip::Compress(const std::string &src, std::string &dest, size_t count)
// {
//     z_stream c_stream;
//     int err = 0;
//     int windowBits = 15;
//     int GZIP_ENCODING = 16;
//     if (src.empty())
//     {
//         return 0;
//     }
//     size_t outputLen =src.size()+count;
//     unique_ptr<char[]> output(new char[outputLen]);
//     c_stream.zalloc = (alloc_func)0;
//     c_stream.zfree = (free_func)0;
//     c_stream.opaque = (voidpf)0;
//     if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//                      windowBits | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY) != Z_OK)
//         return -1;
//     c_stream.next_in = (Bytef *)src.data();
//     c_stream.avail_in = static_cast<decltype(c_stream.avail_in)>(src.size());
//     c_stream.next_out = (Bytef *)output.get();
//     c_stream.avail_out = static_cast<decltype(c_stream.avail_out)>(outputLen);
//     while (c_stream.avail_in != 0 && c_stream.total_out < outputLen)
//     {
//         if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
//             return -1;
//     }
//     if (c_stream.avail_in != 0)
//         return c_stream.avail_in;
//     for (;;)
//     {
//         if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
//             break;
//         if (err != Z_OK)
//             return -1;
//     }
//     if (deflateEnd(&c_stream) != Z_OK)
//         return -1;
//     dest.clear();
//     dest.append(output.get(), c_stream.total_out);
//     return Z_OK;
// }

// int GZip::DeCompress(const char *src, size_t srcLen, const char *dst, size_t dstLen)
// {
//     z_stream strm;
//     strm.zalloc = NULL;
//     strm.zfree = NULL;
//     strm.opaque = NULL;
//     strm.avail_in = static_cast<decltype(strm.avail_in)>(srcLen);
//     strm.avail_out = static_cast<decltype(strm.avail_out)>(dstLen);
//     strm.next_in = (Bytef *)src;
//     strm.next_out = (Bytef *)dst;
//
//     int err = -1, ret = -1;
//     err = inflateInit2(&strm, MAX_WBITS + 16);
//     if (err == Z_OK)
//     {
//         err = inflate(&strm, Z_FINISH);
//         if (err == Z_STREAM_END)
//         {
//             ret = strm.total_out;
//             inflateEnd(&strm);
//             return ret;
//         }
//         else
//         {
//             inflateEnd(&strm);
//             return err;
//         }
//     }
//     else
//     {
//         inflateEnd(&strm);
//         return err;
//     }
// }
    int GZip::DeCompress(const std::string &src, std::string &dest, size_t count) {
        int err = 0;
        if (!src.empty()) {
            size_t outputLen = src.size() + count;
            std::vector<char> output(outputLen);

            z_stream strm;
            memset(&strm, 0, sizeof(strm));
            err = inflateInit2(&strm, MAX_WBITS + 16);
            defer(inflateEnd(&strm));

            if (err == Z_OK) {
                strm.avail_in = static_cast<decltype(z_stream::avail_in)>(src.size());
                strm.next_in = (Bytef *) src.data();
                strm.avail_out = static_cast<decltype(z_stream::avail_out)>(outputLen);
                strm.next_out = (Bytef *) &output[0];

                err = inflate(&strm, Z_FINISH);
                if (err == Z_STREAM_END) {
                    dest.assign(&output[0], &output[0] + strm.total_out);
                    err = Z_OK;
                }
            }
        }
        return err;
    }

    static int GzipCompress(Bytef *src, uLong srcLength, Bytef *dest, uLong &destLength) {
        int ret = 0;
        if (src && srcLength > 0) {
            z_stream c_stream;
            memset(&c_stream, 0, sizeof(c_stream));
            //只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
            if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                             MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
                return -1;
            }
            defer(if (deflateEnd(&c_stream) != Z_OK) { ret = -1; });

            c_stream.next_in = src;
            c_stream.avail_in = srcLength;
            c_stream.next_out = dest;
            c_stream.avail_out = destLength;
            while (c_stream.avail_in != 0 && c_stream.total_out < destLength) {
                if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) {
                    return -1;
                }
            }
            if (c_stream.avail_in != 0) {
                return static_cast<int>(c_stream.avail_in);
            }
            for (;;) {
                int err = deflate(&c_stream, Z_FINISH);
                if (err == Z_STREAM_END) {
                    break;
                }
                if (err != Z_OK) {
                    return -1;
                }
            }

            // if (deflateEnd(&c_stream) != Z_OK) {
            //     return -1;
            // }
            destLength = static_cast<int>(c_stream.total_out);
            ret = 0;
        }
        return ret;
    }

    int GZip::Compress(const string &src, string &dest) {
        auto destLength = static_cast<uLong>(src.size() + 1024);

        for (int times = 0; times <= 2; ++times) {
            std::vector<char> destBody((size_t) destLength);
            int res = GzipCompress((Bytef *) (src.c_str()), static_cast<uLong>(src.size()),
                                   (Bytef *) (&destBody[0]), destLength);
            if (res == 0) {
                dest.assign(&destBody[0], &destBody[0] + destLength);
                return 0;
            }
            destLength <<= 1;
        }
        return -1;
    }
} // namespace common
