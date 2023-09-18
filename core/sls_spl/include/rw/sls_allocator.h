/*
 *  Trade secret of Alibaba Group R&D.
 *  Copyright (c) 2010 Alibaba Group R&D. (unpublished)
 *
 *  All rights reserved.  This notice is intended as a precaution against
 *  inadvertent publication and does not imply publication or any waiver
 *  of confidentiality.  The year included in the foregoing notice is the
 *  year of creation of the work.
 *
 */
#pragma once

#include <vector>
#include <stdint.h>
#include <cstddef>

namespace apsara::sls::spl
{

/**
 * @brief A simple but very fast memory arena.
 *
 * The good usage pattern:
 * 1. Perform many allocations and free them all in a flash.
 * 2. Most of allocations are small (< 1KB), and very few allocations are extremely large.
 * For allocations with this pattern, this memory arena achieves ~50x faster
 * than tcmalloc.
 *
 * Typical Usage:
 *
 * class Faint
 * {
 *     ...
 * };
 *
 * MemoryArena memArena;
 * void* p = memArena.AllocAligned(sizeof(Faint));
 * assert(p != NULL);
 * Faint* obj = new (p) Faint();
 * ...
 * obj->~Faint();
 *
 *
 */
class MemoryArena
{
private:
    static const int32_t kAlignSize = sizeof(void*);
    static const int32_t kPageSize = 4096; // in bytes

public:
    explicit MemoryArena(int32_t firstChunkSize = 4096, int32_t chunkSizeLimit = 1024 * 128)
      : mFirstChunkSize(firstChunkSize),
        mChunkSize(firstChunkSize),
        mChunkSizeLimit(chunkSizeLimit),
        mFreeBytesInChunk(0),
        mAllocPtr(NULL),
        mAllocated(0),
        mUsed(0)
    {
        mAllocPtr = new uint8_t[mChunkSize];
        mAllocatedChunks.push_back(mAllocPtr);
        mFreeBytesInChunk = mChunkSize;
        mAllocated = mChunkSize;
    }

    ~MemoryArena()
    {
        for (; !mAllocatedChunks.empty(); mAllocatedChunks.pop_back()) {
            delete[] mAllocatedChunks.back();
        }
    }

    void Reset(void)
    {
        for(size_t i = 1 ; i < mAllocatedChunks.size() ; i++)
        {
            delete[] mAllocatedChunks[i];
        }
        mAllocatedChunks.resize(1);
        mAllocPtr = mAllocatedChunks[0];
        mChunkSize = mFirstChunkSize;
        mFreeBytesInChunk = mChunkSize;
        mAllocated = mChunkSize;
        mUsed = 0;
    }

    void* AllocAligned(int32_t bytes)
    {
        // Align the alloc size
        int32_t aligned = (bytes + kAlignSize - 1) & ~(kAlignSize - 1);
        return Alloc(aligned);
    }

    int64_t GetUsedSize() const
    {
        return mUsed;
    }

    int64_t GetAllocatedSize() const
    {
        return mAllocated + mAllocatedChunks.size() * sizeof(void*);
    }

private:
    // Please do not make it public, user should always use AllocAligned() to get a better performance.
    // If you have a strong reason to do it, please drop a email to me: shiquan.yangsq@aliyun-inc.com
    uint8_t* Alloc(int32_t bytes)
    {
        uint8_t* mem = NULL;

        if (bytes <= mFreeBytesInChunk) {
            // Alloc it from the free area of the current chunk.
            mem = mAllocPtr;
            mAllocPtr += bytes;
            mFreeBytesInChunk -= bytes;
        } else if (bytes * 2 >= mChunkSize) {
            /*
             * This request is unexpectedly large. We believe the next request
             * will not be so large. Thus, it is wise to allocate it directly
             * from heap in order to avoid polluting chunk size.
             */
            mem = new uint8_t[bytes];
            mAllocatedChunks.push_back(mem);
            mAllocated += bytes;
        } else {
            /*
             * Here we intentionally waste some space in the current chunk.
             */
            if (mChunkSize < mChunkSizeLimit) {
                mChunkSize *= 2;
            }
            mem = new uint8_t[mChunkSize];
            mAllocatedChunks.push_back(mem);
            mAllocPtr = mem + bytes;
            mFreeBytesInChunk = mChunkSize - bytes;
            mAllocated += mChunkSize;
        }

        mUsed += bytes;
        return mem;
    }

private:
    int32_t mFirstChunkSize;
    int32_t mChunkSize;
    int32_t mChunkSizeLimit;
    int32_t mFreeBytesInChunk;
    // Pointing to the available memory address
    uint8_t* mAllocPtr;
    // How many bytes are free in the current chunk
    // The allocated memory chunks.
    std::vector<uint8_t*> mAllocatedChunks;

    // Statistics data
    int64_t mAllocated;
    int64_t mUsed;
private : 
    MemoryArena(const MemoryArena&);
};

class MemoryArenaWithLen
{
private:
    static const int32_t kAlignSize = sizeof(void*);
    static const int32_t kPageSize = 4096; // in bytes

public:
    explicit MemoryArenaWithLen(int32_t chunkSizeLimit = 1024 * 128)
      : mChunkSize(kPageSize),
        mChunkSizeLimit(chunkSizeLimit),
        mAllocPtr(NULL),
        mFreeBytesInChunk(0),
        mAllocated(0),
        mUsed(0)
    {
        mAllocPtr = new uint8_t[mChunkSize];
        mAllocatedChunks.push_back(mAllocPtr);
        mUsedLen.push_back(0);
        mFreeBytesInChunk = mChunkSize;
        mAllocated = mChunkSize;
    }

    ~MemoryArenaWithLen()
    {
        for (; !mAllocatedChunks.empty(); mAllocatedChunks.pop_back()) {
            delete[] mAllocatedChunks.back();
        }
    }

    uint8_t* Alloc(int32_t bytes)
    {
        uint8_t* mem = NULL;
        if (bytes <= mFreeBytesInChunk) {
            mem = mAllocPtr;
            mAllocPtr += bytes;
            mFreeBytesInChunk -= bytes;
            mUsedLen.back() += bytes;
        } else if (bytes * 2 >= mChunkSize) {
            mem = new uint8_t[bytes];
            mAllocatedChunks.push_back(mem);
            mUsedLen.push_back(bytes);
            mAllocated += bytes;
        } else {
            if (mChunkSize < mChunkSizeLimit) {
                mChunkSize *= 2;
            }
            mem = new uint8_t[mChunkSize];
            mAllocatedChunks.push_back(mem);
            mUsedLen.push_back(bytes);
            mAllocPtr = mem + bytes;
            mFreeBytesInChunk = mChunkSize - bytes;
            mAllocated += mChunkSize;
        }
        mUsed += bytes;
        return mem;
    }

    void GetAllLocatedMem(std::vector<uint8_t*>& ptrs, std::vector<int32_t>& lens)
    {
        ptrs = mAllocatedChunks;
        lens = mUsedLen;
    }

    void* AllocAligned(int32_t bytes)
    {
        // Align the alloc size
        int32_t aligned = (bytes + kAlignSize - 1) & ~(kAlignSize - 1);
        return Alloc(aligned);
    }

    int64_t GetUsedSize() const
    {
        return mUsed;
    }

    int64_t GetAllocatedSize() const
    {
        return mAllocated + mAllocatedChunks.size() * sizeof(void*) * 2;
    }

private:
    MemoryArenaWithLen(const MemoryArenaWithLen&);

private:
    int32_t mChunkSize;
    int32_t mChunkSizeLimit;
    // Pointing to the available memory address
    uint8_t* mAllocPtr;
    // How many bytes are free in the current chunk
    int32_t mFreeBytesInChunk;
    // The allocated memory chunks.
    std::vector<uint8_t*> mAllocatedChunks;
    std::vector<int32_t> mUsedLen;
    // Statistics data
    int64_t mAllocated;
    int64_t mUsed;
};


// Destruct function for the POD type.
inline void destruct(char *) {}
inline void destruct(unsigned char *) {}
inline void destruct(short *) {}
inline void destruct(unsigned short *) {}
inline void destruct(int *) {}
inline void destruct(unsigned int *) {}
inline void destruct(long *) {}
inline void destruct(unsigned long *) {}
inline void destruct(long long *) {}
inline void destruct(unsigned long long *) {}
inline void destruct(float *) {}
inline void destruct(double *) {}
inline void destruct(long double *) {}

template <typename T>
inline void destruct(T *p) { p->~T(); }

/**
 * The memory arena allocator follows the C++ standard
 * and is compatible with stl container.
 */
template <typename T, typename ArenaType>
class MemoryArenaAllocator
{
public:
    typedef T                  value_type;
    typedef value_type *       pointer;
    typedef const value_type * const_pointer;
    typedef value_type &       reference;
    typedef const value_type & const_reference;
    typedef std::size_t        size_type;
    typedef std::ptrdiff_t     difference_type;

    explicit MemoryArenaAllocator() throw()
      : mMemArena(NULL)
    {}
    explicit MemoryArenaAllocator(ArenaType& arena) throw()
      : mMemArena(&arena)
    {}
    MemoryArenaAllocator(const MemoryArenaAllocator &other) throw()
      : mMemArena(other.GetMemoryArena())
    {}
    template <typename U>
    MemoryArenaAllocator(const MemoryArenaAllocator<U, ArenaType> &other) throw()
      : mMemArena(other.GetMemoryArena())
    {}
    ~MemoryArenaAllocator() throw()
    {}

    template <typename U>
    struct rebind { typedef MemoryArenaAllocator<U, ArenaType> other; };

    pointer address(reference x) { return &x; }
    const_pointer address(reference x) const { return &x; }

    pointer allocate(size_type n)
    {
        return reinterpret_cast<pointer>(mMemArena->AllocAligned(n * sizeof(value_type)));
    }

    // The deallocate do nothing. All memory is freed while the destruction of the allocator.
    void deallocate(pointer p, size_type n) {}

    void construct(pointer p)
    {
        new (static_cast<void *>(p)) value_type();
    }

    void construct(pointer p, const value_type &x)
    {
        new (static_cast<void *>(p)) value_type(x);
    }

    void destroy(pointer p)
    {
        destruct(p);
    }

    size_type max_size() const throw()
    {
        return static_cast<size_t>(-1) / sizeof(value_type);
    }

    ArenaType * GetMemoryArena() const { return mMemArena; }
    void SetMemoryArena(ArenaType *arena) { mMemArena = arena; }

private:
    ArenaType *mMemArena;
};

// Indicate whether the storage allocated by this allocator can be freed by other one and vice-versa.
// The STL container requires that this function returns true.
template <typename T, typename U, typename ArenaType>
bool operator ==(const MemoryArenaAllocator<T, ArenaType>& a, const MemoryArenaAllocator<U, ArenaType> &b) throw()
{
    return a.GetMemoryArena() == b.GetMemoryArena();
}

template <typename T, typename U, typename ArenaType>
bool operator !=(const MemoryArenaAllocator<T, ArenaType>& a, const MemoryArenaAllocator<U, ArenaType> &b) throw()
{
    return !(a.GetMemoryArena() == b.GetMemoryArena());
}

} // end of namespace apsara
