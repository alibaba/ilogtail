#pragma once

#include "sls_allocator.h"

namespace apsara::sls::spl {
class MemoryBuffer
{
public:
    MemoryBuffer(MemoryArena* arena, int32_t nodeSize = 256, int32_t maxNodeSize = 4096) :
        mArena(arena),
        mBufferHead(NULL),
        mBufferCurrent(NULL),
        mSize(0),
        mNodeSize(nodeSize),
        mMaxNodeSize(maxNodeSize)
    {
    }
    int32_t Size() const
    {
        return mSize;
    }
    void WriteBuffer(const uint8_t* src, int32_t len)
    {
        if (mBufferHead == NULL)
        {
            mBufferHead = new(mArena->AllocAligned(sizeof(BufferNode)))
                BufferNode(mArena, std::max(len, mNodeSize));
            mBufferCurrent = mBufferHead;
            mNodeSize = std::min(mNodeSize * 2, mMaxNodeSize);
        }
        int32_t write_size = mBufferCurrent->write(src, len);
        if (write_size < len)
        {
            mBufferCurrent->mNext = new(mArena->AllocAligned(sizeof(BufferNode)))BufferNode(
                mArena,
                std::max(len - write_size,mNodeSize)
                );
            mBufferCurrent = mBufferCurrent->mNext;
            mNodeSize = std::min(mNodeSize * 2, mMaxNodeSize);
            write_size += mBufferCurrent->write(src + write_size, len - write_size);
        }
        assert(write_size == len);
        mSize += len;
    }

    uint8_t* PackToBuffer(uint8_t* buf) const
    {
        auto bufferNode = mBufferHead;
        while (bufferNode!=NULL)
        {
            buf = bufferNode->PackToBuffer(buf);
            bufferNode = bufferNode->mNext;
        }
        return buf;
    }

private:
    MemoryArena* mArena;
    struct BufferNode {
        BufferNode(MemoryArena* memoryArena, int32_t size) :mNext(NULL), mMaxSize(size), mUsed(0) {
            mData = (uint8_t*)memoryArena->AllocAligned(size);
        }
        uint8_t* mData;
        BufferNode* mNext;
        int32_t mMaxSize;
        int32_t mUsed;
        
        int32_t write(const uint8_t* src, int32_t lenth)
        {
            int32_t write_size = std::min(lenth, (mMaxSize - mUsed));
            memcpy(mData + mUsed, src, write_size);
            mUsed += write_size;
            return write_size;
        }
        uint8_t* PackToBuffer(uint8_t* buf) const
        {
            memcpy(buf,mData,mUsed);
            return buf + mUsed;
        }
    };
    BufferNode* mBufferHead;
    BufferNode* mBufferCurrent;
    int32_t mSize;
    int32_t mNodeSize;
    const int32_t mMaxNodeSize;
};
}