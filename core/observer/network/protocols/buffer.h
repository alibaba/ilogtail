/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
// The buffer data structure inspired by pixeie, so should with pixie license, don't change it.
#pragma once

#include <cstdint>
#include <string>
#include "map"
#include "functional"
#include "interface/type.h"
#include "interface/network.h"
#include "interface/protocol.h"
#include "common/StringPiece.h"
#include "utils.h"
#include "Flags.h"


DECLARE_FLAG_INT32(sls_observer_network_connection_buffer_size);
DECLARE_FLAG_INT32(sls_observer_network_connection_buffer_max_gap);
DECLARE_FLAG_INT32(sls_observer_network_connection_buffer_allow_before_gap_size);

namespace logtail {

class Buffer {
public:
    explicit Buffer(int32_t capacity, int32_t gapSize, int32_t allowBeforeGapSize)
        : mCapacity(capacity), mMaxGapSize(gapSize), mAllowBeforeGapSize(allowBeforeGapSize) {}
    Buffer()
        : mCapacity(INT32_FLAG(sls_observer_network_connection_buffer_size)),
          mMaxGapSize(INT32_FLAG(sls_observer_network_connection_buffer_max_gap)),
          mAllowBeforeGapSize(INT32_FLAG(sls_observer_network_connection_buffer_allow_before_gap_size)) {}


    Buffer(const Buffer& buffer) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /**
     * Add data to the cached buffer.
     * @param pos the pos marked by kernel.
     * @param pkt data
     * @param pktLen  current data len.
     * @param pktRealLen real data len. when the current data len not equals to the real len, must parse the head chunk
     * because waiting is useless.
     * @param timestamp the timestamp marked by kernel.
     */
    BufferResult Add(int32_t pos, uint64_t timestamp, const char* pkt, int32_t pktLen, int32_t pktRealLen);

    StringPiece Head() const { return Get(mPosition); }
    uint64_t GetTimestamp(int32_t pos);
    void RemovePrefix(int32_t len);


    void Reset();
    void ShrinkToFit() { mBuffer.shrink_to_fit(); }
    void CleanupMetadata();
    void CleanupChunks();
    void CleanupTimestamps();
    bool GarbageCollection(uint64_t expireTimeNs);
    void Trim();

private:
    void AddChunk(int32_t pos, int32_t size);
    bool CheckOverlap(int32_t pos, int32_t size);
    void AddTimestamp(int32_t pos, uint64_t timestamp);
    std::map<int32_t, int32_t>::const_iterator GetChunk(int32_t pos) const;
    int32_t EndPosition() const;

    StringPiece Get(int32_t pos) const;

    std::string mBuffer; // store the whole data.
    int32_t mCapacity;
    int32_t mMaxGapSize;
    int32_t mAllowBeforeGapSize;
    int32_t mPosition{0};

    std::map<int32_t, int32_t> mChunks;
    std::map<int32_t, uint64_t> mTimeStamps;
    friend class ContinueBufferTest;
};
} // namespace logtail
