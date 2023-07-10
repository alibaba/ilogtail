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


#include "buffer.h"
#include "Logger.h"
#include "common/Flags.h"

DEFINE_FLAG_INT32(sls_observer_network_connection_buffer_size,
                  "SLS Observer Network per connection buffer size",
                  1024 * 32);
DEFINE_FLAG_INT32(sls_observer_network_connection_buffer_max_gap,
                  "SLS Observer Network per connection buffer max gap size",
                  1024 * 4);
DEFINE_FLAG_INT32(sls_observer_network_connection_buffer_allow_before_gap_size,
                  "SlS Observer Network per connection allow before gap size",
                  1024);

namespace logtail {
template <typename TMapType>
typename TMapType::const_iterator MapLE(const TMapType& map, size_t key) {
    auto iter = map.upper_bound(key);
    if (iter == map.begin()) {
        return map.cend();
    }
    --iter;
    return iter;
}
} // namespace logtail
BufferResult
logtail::Buffer::Add(int32_t pos, uint64_t timestamp, const char* pkt, int32_t pktLen, int32_t pktRealLen) {
    // when found over capacity or cut packet, directly parse.
    if (pktLen > mCapacity || pktLen != pktRealLen) {
        return BufferResult_DirectParse;
    }

    int32_t fPos = pos - mPosition;
    int32_t bPos = pos + pktLen - mPosition;
    bool cleanup = false;
    if (fPos < 0 && bPos == 0) {
        // means directly drop
        return BufferResult_Drop;
    }
    if (bPos < 0) {
        // skip old packet.
        LOG_TRACE(sLogger,
                  ("skip old packet because older than head pos, current_pos",
                   pos)("current_tail_pos", pos + pktLen)("head_pos", mPosition));
        return BufferResult_Drop;
    } else if (fPos < 0) {
        int prefix = 0 - fPos;
        pkt += prefix;
        pos += prefix;
        fPos = 0;
        LOG_TRACE(sLogger,
                  ("find partially packet, current_pod",
                   pos)("current_tail_pos", pos + pktLen)("head_pos", mPosition)("remove_prefix", prefix));

    } else if (bPos > static_cast<int32_t>(mBuffer.size())) {
        // means buffer cannot contains the input packet, direct try parse.
        // when parse fail, direct drop.
        if (pos > EndPosition() + mMaxGapSize) {
            // means should drop history data.
            mPosition = pos - mAllowBeforeGapSize;
            fPos = mAllowBeforeGapSize;
            bPos = mAllowBeforeGapSize + pktLen;
            cleanup = true;
        }

        int32_t size = pos - mPosition + pktLen;
        if (size > mCapacity) {
            int32_t removed = size - mCapacity;
            mBuffer.erase(0, removed);
            mPosition += removed;
            fPos -= removed;
            bPos -= removed;
            cleanup = true;
        }
        mBuffer.resize(bPos);
    }

    if (CheckOverlap(pos, pktLen)) {
        return BufferResult_Drop;
    }
    memcpy((void*)(mBuffer.data() + fPos), pkt, pktLen);
    AddChunk(pos, pktLen);
    AddTimestamp(pos, timestamp);
    if (cleanup)
        CleanupMetadata();
    return BufferResult_Success;
}
int32_t logtail::Buffer::EndPosition() const {
    int32_t end = mPosition;
    if (!mChunks.empty()) {
        auto last = std::prev(mChunks.end());
        end = last->first + last->second;
    }
    return end;
}
void logtail::Buffer::AddTimestamp(int32_t pos, uint64_t timestamp) {
    this->mTimeStamps[pos] = timestamp;
}
void logtail::Buffer::AddChunk(int32_t pos, int32_t size) {
    auto riter = mChunks.lower_bound(pos);
    auto liter = riter;
    if (liter != mChunks.begin()) {
        --liter;
    }

    bool lfull = liter != mChunks.end() ? liter->first + liter->second == pos : false;
    bool rfull = riter != mChunks.end() ? pos + size == riter->first : false;

    if (lfull && rfull) {
        liter->second += size + riter->second;
        mChunks.erase(riter);
    } else if (lfull) {
        liter->second += size;
    } else if (rfull) {
        mChunks[pos] = size + riter->second;
        mChunks.erase(riter);
    } else {
        mChunks[pos] = size;
    }
}

bool logtail::Buffer::CheckOverlap(int32_t pos, int32_t size) {
    auto riter = mChunks.lower_bound(pos);
    bool roverlap = riter != mChunks.end() ? pos + size > riter->first : false;
    bool loverlap = false;
    if (riter != mChunks.begin()) {
        --riter;
        loverlap = pos < riter->first + riter->second;
    }
    return roverlap || loverlap;
}

void logtail::Buffer::Reset() {
    mBuffer.clear();
    mChunks.clear();
    mTimeStamps.clear();
    mPosition = 0;
    ShrinkToFit();
}

logtail::StringPiece logtail::Buffer::Get(int32_t pos) const {
    auto iter = GetChunk(pos);
    if (iter == mChunks.end()) {
        return {};
    }
    uint32_t bytes = iter->first + iter->second - pos;
    if (bytes <= 0) {
        return {};
    }
    int32_t head = pos - mPosition;
    if (head < 0 || head > static_cast<int32_t>(mBuffer.size())) {
        return {};
    }
    return {mBuffer.data() + head, bytes};
}

std::map<int32_t, int32_t>::const_iterator logtail::Buffer::GetChunk(int32_t pos) const {
    auto iter = MapLE(mChunks, pos);
    if (mChunks.cend() == iter) {
        return mChunks.cend();
    }
    if (iter->first <= pos && iter->first + iter->second - pos <= 0) {
        return mChunks.end();
    }
    return iter;
}
void logtail::Buffer::CleanupMetadata() {
    CleanupChunks();
    CleanupTimestamps();
}
void logtail::Buffer::CleanupChunks() {
    auto iter = MapLE(mChunks, mPosition);
    if (iter == mChunks.cend()) {
        return;
    }
    int32_t bytes = iter->first + iter->second - mPosition;
    ++iter;
    mChunks.erase(mChunks.begin(), iter);
    if (bytes > 0) {
        mChunks.insert(std::make_pair(mPosition, bytes));
    }
    if (mChunks.empty()) {
        mBuffer.clear();
    }
}

void logtail::Buffer::CleanupTimestamps() {
    auto iter = MapLE(mTimeStamps, mPosition);
    if (iter == mTimeStamps.cend()) {
        return;
    }
    mTimeStamps.erase(mTimeStamps.begin(), iter);
}
bool logtail::Buffer::GarbageCollection(uint64_t expireTimeNs) {
    if (this->mTimeStamps.empty()) {
        return true;
    }
    auto iter = mTimeStamps.begin();
    while (iter != mTimeStamps.end()) {
        if (iter->second < expireTimeNs) {
            ++iter;
            continue;
        }
        break;

    }
    if (iter != mTimeStamps.end()) {
        RemovePrefix(iter->first - mPosition);
    } else {
        this->Reset();
    }
    return this->mBuffer.empty();
}


void logtail::Buffer::RemovePrefix(int32_t len) {
    if (len <= 0) {
        return;
    }
    mBuffer.erase(0, len);
    mPosition += len;
    CleanupMetadata();
}
void logtail::Buffer::Trim() {
    if (mChunks.empty()) {
        return;
    }
    auto& chunk_pos = mChunks.begin()->first;
    int32_t trim_size = chunk_pos - mPosition;
    if (trim_size > 0) {
        mBuffer.erase(0, trim_size);
        mPosition += trim_size;
    }
}
uint64_t logtail::Buffer::GetTimestamp(int32_t pos) {
    if (GetChunk(pos) == mChunks.end()) {
        return -1;
    }
    auto iter = MapLE(mTimeStamps, pos);
    if (iter == mTimeStamps.end()) {
        return -1;
    }
    return iter->second;
}