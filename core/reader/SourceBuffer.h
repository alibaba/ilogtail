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

#include <memory>
#include <list>
#include <boost/utility/string_view.hpp>

namespace logtail {

// like string, in string_view, tailing \0 is not included in size
typedef boost::string_view StringView;

struct StringBuffer {
    StringBuffer(char* data, size_t capacity) : data(data), size(0), capacity(capacity) { data[0] = '\0'; }
    char* const data;
    size_t size;
    const size_t capacity; // max bytes of data can be stored, data[capacity] is always '\0'.
};

class BufferAllocator {
public:
    BufferAllocator() : mSize(0), mUsed(0), mOtherUsed(0) {}

    bool Init(size_t size) {
        if (size == 0 || mSize > 0) {
            return false;
        }
        mData.reset(new char[size]);
        mSize = size;
        return true;
    }

    bool IsInited() { return mSize > 0; }

    void* Allocate(size_t size) {
        if (mSize == 0) {
            return nullptr;
        }
        if (mUsed + size > mSize) {
            mOtherData.emplace_back(new char[size]);
            return mOtherData.back().get();
        }
        void* result = mData.get() + mUsed;
        mUsed += size;
        return result;
    }

    size_t TotalAllocated() { return mUsed + mOtherUsed; }

    size_t TotalMemory() { return mSize + mOtherUsed; }

private:
    std::unique_ptr<char[]> mData;
    size_t mSize;
    size_t mUsed;
    // Stores other memory allocation than buffer
    std::list<std::unique_ptr<char[]>> mOtherData;
    size_t mOtherUsed;
};

class SourceBuffer {
public:
    SourceBuffer() {}
    StringBuffer AllocateStringBuffer(size_t size) {
        if (!mAllocator.IsInited()) {
            mAllocator.Init(size * 1.2); // TODO: better allocate strategy
        }
        char* data = static_cast<char*>(mAllocator.Allocate(size + 1));
        data[size] = '\0';
        return StringBuffer(data, size);
    }
    // StringView GetProperty(const char* key){};

private:
    BufferAllocator mAllocator;
};

} // namespace logtail