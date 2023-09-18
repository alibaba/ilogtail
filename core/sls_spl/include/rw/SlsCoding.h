#pragma once

#include "string"

namespace apsara::sls::spl {

extern char* EncodeVarint64(char* dst, uint64_t v);
extern char* EncodeVarint32(char* dst, uint32_t v);

extern void PutVarint32(std::string* dst, uint32_t value);
extern void PutVarint64(std::string* dst, uint64_t value);

extern void AppendStringToPBRawString(std::string* dest, const std::string* str, const int number);

extern const char* GetVarint32Ptr_old(const char* p,const char* limit, uint32_t* v);

// Pointer-based variants of GetVarint...  These either store a value
// in *v and return a pointer just past the parsed value, or return
// NULL on error.  These routines only look at bytes in the range
// [p..limit-1]
inline static const char* GetVarint32Ptr(const char* p,const char* limit, uint32_t* v)
{
#define BITS_VALUE(ptr, offset)   (((uint32_t)(*(ptr))) << offset)
#define BITS_VALUE_WITH_MASK(ptr, offset)   (((uint32_t)(((uint8_t)(*(ptr))) ^ MASK)) << offset)
    if (p == NULL) 
    {
        return NULL;
    }
    const static uint8_t MASK = 128;
    if (p < limit && ((*p) & MASK) == 0) 
    {
        *v = *p;
        return p + 1;
    }
    else if (p + 1 < limit && !((*(p + 1)) & MASK))
    {
        *v = BITS_VALUE_WITH_MASK(p, 0) |  BITS_VALUE(p + 1 , 7);
        return p + 2;
    }
    else if (p + 2 < limit && !((*(p + 2)) & MASK))
    {
        *v = BITS_VALUE_WITH_MASK(p, 0) |  BITS_VALUE_WITH_MASK(p + 1 , 7) | BITS_VALUE(p + 2 , 14);
        return p + 3;
    }
    else if (p + 3 < limit && !((*(p + 3)) & MASK))
    {
        *v = BITS_VALUE_WITH_MASK(p, 0) |  BITS_VALUE_WITH_MASK(p + 1 , 7) | BITS_VALUE_WITH_MASK(p + 2 , 14) | BITS_VALUE( p + 3 , 21);
        return p + 4;
    }
    else if (p + 4 < limit && !((*(p + 4)) & MASK))
    {
        *v = BITS_VALUE_WITH_MASK(p, 0) |  BITS_VALUE_WITH_MASK(p + 1 , 7) | BITS_VALUE_WITH_MASK(p + 2 , 14) | BITS_VALUE_WITH_MASK( p + 3 , 21) | BITS_VALUE(p + 4 , 28);
        return p + 5;
    }
    *v = 0;
    return NULL;

#undef BITS_VALUE
#undef BITS_VALUE_WITH_MASK
}
extern const char* GetVarint64Ptr(const char* p,const char* limit, uint64_t* v); 

// Returns the length of the varint32 or varint64 encoding of "v"
extern int VarintLength(uint64_t v); 



}
