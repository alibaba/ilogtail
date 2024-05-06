/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <list>
#include <memory>

#include "models/StringView.h"

namespace logtail {

class StringBuffer {
    friend class SourceBuffer;

public:
    bool IsValid() { return data != nullptr; }

    char* const data;
    size_t size;
    const size_t capacity; // max bytes of data can be stored, data[capacity] is always '\0'.

private:
    // capacity is guranteed to be greater than 0
    StringBuffer(char* data, size_t capacity) : data(data), size(0), capacity(capacity) { data[0] = '\0'; }
};

// only movable
class BufferAllocator {
private:
    static const uint32_t kAlignSize = sizeof(void*);
    static const uint32_t kPageSize = 4096; // in bytes

public:
    explicit BufferAllocator(uint32_t firstChunkSize = 4096, uint32_t chunkSizeLimit = 1024 * 128)
        : mFirstChunkSize(firstChunkSize), mChunkSizeLimit(chunkSizeLimit), mChunkSize(firstChunkSize) {
        mAllocPtr = new uint8_t[mChunkSize];
        mAllocatedChunks.push_back(mAllocPtr);
        mFreeBytesInChunk = mChunkSize;
        mAllocated = mChunkSize;
    }

    BufferAllocator(const BufferAllocator&) = delete;
    BufferAllocator& operator=(const BufferAllocator&) = delete;

    BufferAllocator(BufferAllocator&&) = default;
    BufferAllocator& operator=(BufferAllocator&&) = default;

    ~BufferAllocator() {
        for (size_t i = 0; i < mAllocatedChunks.size(); i++) {
            delete[] mAllocatedChunks[i];
        }
    }

    void Reset(void) {
        for (size_t i = 1; i < mAllocatedChunks.size(); i++) {
            delete[] mAllocatedChunks[i];
        }
        mAllocatedChunks.resize(1);
        mAllocPtr = mAllocatedChunks[0];
        mChunkSize = mFirstChunkSize;
        mFreeBytesInChunk = mChunkSize;
        mAllocated = mChunkSize;
        mUsed = 0;
    }

    void* Allocate(uint32_t bytes) {
        // Align the alloc size
        int32_t aligned = (bytes + kAlignSize - 1) & ~(kAlignSize - 1);
        return Alloc(aligned);
    }

    int64_t GetUsedSize() const { return mUsed; }

    size_t TotalAllocated() { return mAllocated; }

    int64_t GetAllocatedSize() const { return mAllocated + mAllocatedChunks.size() * sizeof(void*); }

private:
    // Please do not make it public, user should always use Allocate() to get a better performance.
    // If you have a strong reason to do it, please drop a email to me: shiquan.yangsq@aliyun-inc.com
    uint8_t* Alloc(uint32_t bytes) {
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
    const uint32_t mFirstChunkSize = 4096;
    const uint32_t mChunkSizeLimit = 1024 * 128;

    // The allocated memory chunks
    std::vector<uint8_t*> mAllocatedChunks;
    // Statistics data
    uint64_t mAllocated = 0;
    uint64_t mUsed = 0;

    // Pointing to the available memory address
    uint8_t* mAllocPtr = nullptr;
    // How many bytes are free in the current chunk
    uint32_t mFreeBytesInChunk = 0;
    // Current chunk size
    uint32_t mChunkSize = 0;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class SourceBufferUnittest;
#endif
};

// only movable
class SourceBuffer {
public:
    StringBuffer AllocateStringBuffer(size_t size) {
        char* data = static_cast<char*>(mAllocator.Allocate(size + 1));
        data[size] = '\0';
        return StringBuffer(data, size);
    }

    StringBuffer CopyString(const char* data, size_t len) {
        StringBuffer sb = AllocateStringBuffer(len);
        memcpy(sb.data, data, len);
        sb.size = len;
        return sb;
    }
    StringBuffer CopyString(const std::string& s) { return CopyString(s.data(), s.length()); }
    StringBuffer CopyString(StringView s) { return CopyString(s.data(), s.length()); }

private:
    BufferAllocator mAllocator;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class LogEventUnittest;
    friend class PipelineEventGroupUnittest;
#endif
};

} // namespace logtail