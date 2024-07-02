/*
 * Copyright 2022 iLogtail Authors
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
#include <stdio.h>

#include <deque>
#include <string>
#include <unordered_map>

#include "Lock.h"
#include "common/FeedbackInterface.h"
#include "common/LogstoreFeedbackKey.h"
#include "common/LogstoreFeedbackQueue.h"
#include "logger/Logger.h"
#include "queue/SenderQueueItem.h"
#include "sender/ConcurrencyLimiter.h"
#include "sender/SenderQueueParam.h"

namespace logtail {

class FlusherSLS;

struct LogstoreSenderStatistics {
    LogstoreSenderStatistics();
    void Reset();

    int32_t mMaxUnsendTime;
    int32_t mMinUnsendTime;
    int32_t mMaxSendSuccessTime;
    uint32_t mSendQueueSize;
    uint32_t mSendNetWorkErrorCount;
    uint32_t mSendQuotaErrorCount;
    uint32_t mSendDiscardErrorCount;
    uint32_t mSendSuccessCount;
    bool mSendBlockFlag;
    bool mValidToSendFlag;
};

struct LogstoreSenderInfo {
    enum SendResult {
        SendResult_OK,
        SendResult_Buffered, // move to file buffer
        SendResult_NetworkFail,
        SendResult_QuotaFail,
        SendResult_DiscardFail,
        SendResult_OtherFail
    };

    std::string mRegion;
    volatile int32_t mLastNetworkErrorCount;
    volatile int32_t mLastQuotaExceedCount;
    volatile int32_t mLastNetworkErrorTime;
    volatile int32_t mLastQuotaExceedTime;
    volatile double mNetworkRetryInterval;
    volatile double mQuotaRetryInterval;
    volatile bool mNetworkValidFlag;
    volatile bool mQuotaValidFlag;
    volatile int32_t mSendConcurrency;
    volatile int32_t mSendConcurrencyUpdateTime;

    LogstoreSenderInfo();

    void SetRegion(const std::string& region);

    bool ConcurrencyValid();
    void ConcurrencyDec() { --mSendConcurrency; }

    bool CanSend(int32_t curTime);
    // RecordSendResult
    // @return true if need to trigger.
    bool RecordSendResult(SendResult rst, LogstoreSenderStatistics& statisticsItem);
    // return value, recover sucess flag(if logstore is invalid before, return true, else return false)
    bool OnRegionRecover(const std::string& region);
};

template <class PARAM>
class SingleLogstoreSenderManager : public SingleLogstoreFeedbackQueue<SenderQueueItem*, PARAM> {
public:
    SingleLogstoreSenderManager()
        : mLastSendTimeSecond(0), mLastSecondTotalBytes(0), mMaxSendBytesPerSecond(-1), mFlowControlExpireTime(0) {}

    void SetMaxSendBytesPerSecond(int32_t maxBytes, int32_t expireTime) {
        mMaxSendBytesPerSecond = maxBytes;
        mFlowControlExpireTime = expireTime;
    }

    void GetAllIdleLoggroup(std::vector<SenderQueueItem*>& logGroupVec) {
        if (this->mSize == 0) {
            return;
        }

        uint64_t index = QueueType::ExactlyOnce == this->mType ? 0 : this->mRead;
        const uint64_t endIndex = QueueType::ExactlyOnce == this->mType ? this->SIZE : this->mWrite;
        for (; index < endIndex; ++index) {
            SenderQueueItem* item = this->mArray[index % this->SIZE];
            if (item == NULL) {
                continue;
            }
            if (item->mStatus == SendingStatus::IDLE) {
                item->mStatus = SendingStatus::SENDING;
                logGroupVec.push_back(item);
            }
        }
    }

    void GetAllIdleLoggroupWithLimit(std::vector<SenderQueueItem*>& logGroupVec, int32_t nowTime) {
        bool expireFlag = (mFlowControlExpireTime > 0 && nowTime > mFlowControlExpireTime);
        if (this->mSize == 0 || (!expireFlag && mMaxSendBytesPerSecond == 0)) {
            return;
        }
        if (!expireFlag && mMaxSendBytesPerSecond > 0 && nowTime != mLastSendTimeSecond) {
            mLastSecondTotalBytes = 0;
            mLastSendTimeSecond = nowTime;
        }

        uint64_t index = QueueType::ExactlyOnce == this->mType ? 0 : this->mRead;
        const uint64_t endIndex = QueueType::ExactlyOnce == this->mType ? this->SIZE : this->mWrite;
        for (; index < endIndex; ++index) {
            SenderQueueItem* item = this->mArray[index % this->SIZE];
            if (item == NULL) {
                continue;
            }
            for (auto& limiter : mLimiters) {
                if (!limiter->IsValidToPop(mSenderInfo.mRegion)) {
                    return;
                }
            }
            if (item->mStatus == SendingStatus::IDLE) {
                // check consurrency
                // check first, when mMaxSendBytesPerSecond is 1000, and the packet size is 10K, we should send this
                // packet. if not, this logstore will block
                if ((!mSenderInfo.ConcurrencyValid())
                    || (!expireFlag && mMaxSendBytesPerSecond > 0 && mLastSecondTotalBytes > mMaxSendBytesPerSecond)) {
                    return;
                }
                mSenderInfo.ConcurrencyDec();
                mLastSecondTotalBytes += item->mRawSize;
                item->mStatus = SendingStatus::SENDING;
                logGroupVec.push_back(item);
                for (auto& limiter : mLimiters) {
                    limiter->PostPop(mSenderInfo.mRegion);
                }
            }
        }
    }

    bool insertExactlyOnceItem(SLSSenderQueueItem* item) {
        auto& eo = item->mExactlyOnceCheckpoint;
        if (eo->IsComplete()) {
            // Checkpoint is complete, which means it is a replayed checkpoint,
            //  use it directly.
            if (this->mArray[eo->index] != NULL) {
                APSARA_LOG_ERROR(sLogger,
                                 ("replay checkpoint has been used",
                                  "unexpected")("key", eo->key)("checkpoint", eo->data.DebugString()));
                return false;
            }
            this->mArray[eo->index] = item;
        } else {
            // Incomplete checkpoint, select an available checkpoint for it.
            // Use mWrite to record last selected index, iterate from it for balance.
            this->mWrite++;
            for (size_t idx = 0; idx < this->SIZE; ++idx, this->mWrite++) {
                auto index = this->mWrite % this->SIZE;
                if (this->mArray[index] != NULL) {
                    continue;
                }
                this->mArray[index] = item;
                auto& newCpt = mRangeCheckpoints[index];
                newCpt->data.set_read_offset(eo->data.read_offset());
                newCpt->data.set_read_length(eo->data.read_length());
                eo = newCpt;
                break;
            }
            if (!eo->IsComplete()) {
                this->mValid = false;
                mExtraBuffers.push_back(item);
                APSARA_LOG_DEBUG(sLogger,
                                 ("can not find any checkpoint for data",
                                  "add to extra buffer")("fb key", eo->fbKey)("checkpoint", eo->data.DebugString())(
                                     "queue size", this->mSize)("buffer size", mExtraBuffers.size()));
                return true;
            }
        }

        this->mSize++;
        if (this->IsFull()) {
            this->mValid = false;
        }
        eo->Prepare();
        APSARA_LOG_DEBUG(
            sLogger,
            ("bind data with checkpoint", eo->index)("checkpoint", eo->data.DebugString())("queue size", this->mSize));
        return true;
    }

    // with empty item
    bool InsertItem(SenderQueueItem* item, const std::string& region) {
        if (!region.empty()) {
            mSenderInfo.SetRegion(region);
        }
        if (QueueType::ExactlyOnce == this->mType) {
            return insertExactlyOnceItem(static_cast<SLSSenderQueueItem*>(item));
        }

        if (this->IsFull()) {
            return false;
        }
        auto index = this->mRead;
        for (; index < this->mWrite; ++index) {
            if (this->mArray[index % this->SIZE] == NULL) {
                break;
            }
        }
#ifdef LOGTAIL_DEBUG_FLAG
        APSARA_LOG_DEBUG(sLogger,
                         ("Insert send item",
                          item)("size", this->mSize)("read", this->mRead)("write", this->mWrite)("index", index));
#endif
        this->mArray[index % this->SIZE] = item;
        if (index == this->mWrite) {
            ++this->mWrite;
        }
        ++this->mSize;
        if (this->mSize == this->HIGH_SIZE) {
            this->mValid = false;
        }
        return true;
    }

    int32_t removeExactlyOnceItem(SLSSenderQueueItem* item) {
        auto& checkpoint = item->mExactlyOnceCheckpoint;
        this->mArray[checkpoint->index] = NULL;
        this->mSize--;
        delete item;
        if (!mExtraBuffers.empty()) {
            auto extraItem = mExtraBuffers.front();
            mExtraBuffers.pop_front();
            auto& eo = extraItem->mExactlyOnceCheckpoint;
            APSARA_LOG_DEBUG(sLogger,
                             ("process extra item at first",
                              "after item removed")("fb key", eo->fbKey)("checkpoint", eo->data.DebugString()));
            insertExactlyOnceItem(extraItem);
            return 1;
        }
        this->mValid = true;
        return 2;
    }

    // with empty item
    int32_t RemoveItem(SenderQueueItem* item, bool deleteFlag) {
        if (this->IsEmpty()) {
            return 0;
        }

        if (QueueType::ExactlyOnce == this->mType) {
            return removeExactlyOnceItem(static_cast<SLSSenderQueueItem*>(item));
        }

        auto index = this->mRead;
        for (; index < this->mWrite; ++index) {
            auto dataItem = this->mArray[index % this->SIZE];
            if (dataItem == item) {
                if (deleteFlag) {
                    delete dataItem;
                }
#ifdef LOGTAIL_DEBUG_FLAG
                APSARA_LOG_DEBUG(sLogger,
                                 ("Remove send item", dataItem)("size", this->mSize)("read", this->mRead)(
                                     "write", this->mWrite)("index", index));
#endif
                this->mArray[index % this->SIZE] = NULL;
                break;
            }
        }
        if (index == this->mWrite) {
            APSARA_LOG_ERROR(
                sLogger, ("find no sender item", "")("read", this->mRead)("write", this->mWrite)("size", this->mSize));
            return 0;
        }
        // need to check mWrite to avoid dead while
        while (this->mRead < this->mWrite && this->mArray[this->mRead % this->SIZE] == NULL) {
            ++this->mRead;
        }
        int rst = 1;
        if (--this->mSize == this->LOW_SIZE && !this->mValid) {
            this->mValid = true;
            rst = 2;
        }
        return rst;
    }

    LogstoreSenderStatistics GetSenderStatistics() {
        LogstoreSenderStatistics statisticsItem = mSenderStatistics;
        mSenderStatistics.Reset();

        statisticsItem.mSendBlockFlag = !this->IsValid();
        statisticsItem.mValidToSendFlag = IsValidToSend(time(NULL));
        if (this->IsEmpty()) {
            return statisticsItem;
        }

        uint64_t index = QueueType::ExactlyOnce == this->mType ? 0 : this->mRead;
        int32_t minSendTime = INT32_MAX;
        int32_t maxSendTime = 0;
        for (size_t i = 0; i < this->mSize; ++i, ++index) {
            SenderQueueItem* item = this->mArray[index % this->SIZE];
            if (item == NULL) {
                continue;
            }

            if (item->mEnqueTime < minSendTime) {
                minSendTime = item->mEnqueTime;
            }
            if (item->mEnqueTime > maxSendTime) {
                maxSendTime = item->mEnqueTime;
            }
            ++statisticsItem.mSendQueueSize;
        }
        if (statisticsItem.mSendQueueSize > 0) {
            statisticsItem.mMinUnsendTime = minSendTime;
            statisticsItem.mMaxUnsendTime = maxSendTime;
        }
        return statisticsItem;
    }

    int32_t OnSendDone(SenderQueueItem* item, LogstoreSenderInfo::SendResult sendRst, bool& needTrigger) {
        needTrigger = mSenderInfo.RecordSendResult(sendRst, mSenderStatistics);
        // if (!mSenderInfo.mNetworkValidFlag) {
        //     LOG_WARNING(sLogger,
        //                 ("Network fail, pause logstore", flusher->mLogstore)("project", flusher->mProject)(
        //                     "region", mSenderInfo.mRegion)("retry interval", mSenderInfo.mNetworkRetryInterval));
        // }
        // if (!mSenderInfo.mQuotaValidFlag) {
        //     LOG_WARNING(sLogger,
        //                 ("Quota fail, pause logstore", flusher->mLogstore)("project", flusher->mProject)(
        //                     "region", mSenderInfo.mRegion)("retry interval", mSenderInfo.mQuotaRetryInterval));
        // }

        // if send error, reset status to idle, and wait to send again
        // network fail or quota fail
        if (sendRst != LogstoreSenderInfo::SendResult_OK && sendRst != LogstoreSenderInfo::SendResult_Buffered
            && sendRst != LogstoreSenderInfo::SendResult_DiscardFail) {
            item->mStatus = SendingStatus::IDLE;
            return 0;
        }
        if (mSenderStatistics.mMaxSendSuccessTime < item->mEnqueTime) {
            mSenderStatistics.mMaxSendSuccessTime = item->mEnqueTime;
        }
        // else remove item except buffered
        return RemoveItem(item, sendRst != LogstoreSenderInfo::SendResult_Buffered);
    }

    bool IsValidToSend(int32_t curTime) { return mSenderInfo.CanSend(curTime); }

    LogstoreSenderInfo mSenderInfo;
    LogstoreSenderStatistics mSenderStatistics;

    // add for flow control
    volatile int32_t mLastSendTimeSecond;
    volatile int32_t mLastSecondTotalBytes;

    volatile int32_t mMaxSendBytesPerSecond; // <0, no flowControl, 0 pause sending,
    volatile int32_t mFlowControlExpireTime; // <=0 no expire

    std::vector<RangeCheckpointPtr> mRangeCheckpoints;
    std::deque<SLSSenderQueueItem*> mExtraBuffers;

    std::vector<Limiter*> mLimiters;
};

template <class PARAM>
class LogstoreSenderQueue : public FeedbackInterface {
protected:
    typedef SingleLogstoreSenderManager<PARAM> SingleLogStoreManager;
    typedef std::unordered_map<LogstoreFeedBackKey, SingleLogStoreManager> LogstoreFeedBackQueueMap;
    typedef typename std::unordered_map<LogstoreFeedBackKey, SingleLogStoreManager>::iterator
        LogstoreFeedBackQueueMapIterator;

public:
    LogstoreSenderQueue() : mFeedBackObj(NULL), mUrgentFlag(false), mSenderQueueBeginIndex(0) {}

    void SetParam(const size_t lowSize, const size_t highSize, const size_t maxSize) {
        PARAM* pParam = PARAM::GetInstance();
        pParam->SetLowSize(lowSize);
        pParam->SetHighSize(highSize);
        pParam->SetMaxSize(maxSize);
    }

    void SetFeedBackObject(FeedbackInterface* pFeedbackObj) {
        PTScopedLock dataLock(mLock);
        mFeedBackObj = pFeedbackObj;
    }

    void Signal() { mTrigger.Trigger(); }

    void Feedback(int64_t key) override { mTrigger.Trigger(); }

    bool IsValidToPush(int64_t key) const override {
        PTScopedLock dataLock(mLock);
        auto iter = mLogstoreSenderQueueMap.find(key);
        if (iter == mLogstoreSenderQueueMap.end()) {
            // this only happens when go self telemetry is sent to sls
            return true;
        }

        // For correctness, exactly once queue should ignore mUrgentFlag.
        if (iter->second.GetQueueType() == QueueType::ExactlyOnce) {
            return iter->second.IsValid();
        }

        if (mUrgentFlag) {
            return true;
        }
        return iter->second.IsValid();
    }

    void SetLogstoreFlowControl(const LogstoreFeedBackKey& key, int32_t maxBytes, int32_t expireTime) {
        PTScopedLock dataLock(mLock);
        SingleLogStoreManager& singleQueue = mLogstoreSenderQueueMap[key];
        singleQueue.SetMaxSendBytesPerSecond(maxBytes, expireTime);
    }

    void ConvertToExactlyOnceQueue(const LogstoreFeedBackKey& key, const std::vector<RangeCheckpointPtr>& checkpoints) {
        PTScopedLock dataLock(mLock);
        auto& queue = mLogstoreSenderQueueMap[key];
        queue.mRangeCheckpoints = checkpoints;
        queue.ConvertToExactlyOnceQueue(0, checkpoints.size(), checkpoints.size());
    }

    bool Wait(int32_t waitMs) { return mTrigger.Wait(waitMs); }

    bool IsValid(const LogstoreFeedBackKey& key) {
        PTScopedLock dataLock(mLock);
        SingleLogStoreManager& singleQueue = mLogstoreSenderQueueMap[key];
        return singleQueue.IsValid();
    }

    bool PushItem(const LogstoreFeedBackKey& key, SenderQueueItem* const& item, const std::string& region = "") {
        {
            PTScopedLock dataLock(mLock);
            SingleLogStoreManager& singleQueue = mLogstoreSenderQueueMap[key];
            if (!singleQueue.InsertItem(item, region)) {
                return false;
            }
        }
        Signal();
        return true;
    }

    void CheckAndPopAllItem(std::vector<SenderQueueItem*>& itemVec, int32_t curTime, bool& singleQueueFullFlag) {
        singleQueueFullFlag = false;
        PTScopedLock dataLock(mLock);
        if (mLogstoreSenderQueueMap.empty()) {
            return;
        }

        // must check index before moving iterator
        mSenderQueueBeginIndex = mSenderQueueBeginIndex % mLogstoreSenderQueueMap.size();

        // here we set sender queue begin index, let the sender order be different each time
        LogstoreFeedBackQueueMapIterator beginIter = mLogstoreSenderQueueMap.begin();
        std::advance(beginIter, mSenderQueueBeginIndex++);

        PopItem(beginIter, mLogstoreSenderQueueMap.end(), itemVec, curTime, singleQueueFullFlag);
        PopItem(mLogstoreSenderQueueMap.begin(), beginIter, itemVec, curTime, singleQueueFullFlag);
    }

    static void PopItem(LogstoreFeedBackQueueMapIterator beginIter,
                        LogstoreFeedBackQueueMapIterator endIter,
                        std::vector<SenderQueueItem*>& itemVec,
                        int32_t curTime,
                        bool& singleQueueFullFlag) {
        for (LogstoreFeedBackQueueMapIterator iter = beginIter; iter != endIter; ++iter) {
            SingleLogStoreManager& singleQueue = iter->second;
            if (!singleQueue.IsValidToSend(curTime)) {
                continue;
            }
            singleQueue.GetAllIdleLoggroupWithLimit(itemVec, curTime);
            singleQueueFullFlag |= !singleQueue.IsValid();
        }
    }

    void PopAllItem(std::vector<SenderQueueItem*>& itemVec, int32_t curTime, bool& singleQueueFullFlag) {
        singleQueueFullFlag = false;
        PTScopedLock dataLock(mLock);
        for (LogstoreFeedBackQueueMapIterator iter = mLogstoreSenderQueueMap.begin();
             iter != mLogstoreSenderQueueMap.end();
             ++iter) {
            iter->second.GetAllIdleLoggroup(itemVec);
            singleQueueFullFlag |= !iter->second.IsValid();
        }
    }

    void OnLoggroupSendDone(SenderQueueItem* item, LogstoreSenderInfo::SendResult sendRst) {
        if (item == NULL) {
            return;
        }

        LogstoreFeedBackKey key = item->mQueueKey;
        int rst = 0;
        bool needTrigger = false;
        {
            PTScopedLock dataLock(mLock);
            SingleLogStoreManager& singleQueue = mLogstoreSenderQueueMap[key];
            rst = singleQueue.OnSendDone(item, sendRst, needTrigger);
        }
        if (rst == 2 && mFeedBackObj != NULL) {
            APSARA_LOG_DEBUG(sLogger, ("OnLoggroupSendDone feedback", ""));
            mFeedBackObj->Feedback(key);
        }
        if (needTrigger) {
            // trigger deamon sender
            Signal();
        }
    }

    void OnRegionRecover(const std::string& region) {
        APSARA_LOG_DEBUG(sLogger, ("Recover region", region));
        bool recoverFlag = false;
        PTScopedLock dataLock(mLock);
        for (LogstoreFeedBackQueueMapIterator iter = mLogstoreSenderQueueMap.begin();
             iter != mLogstoreSenderQueueMap.end();
             ++iter) {
            recoverFlag |= iter->second.mSenderInfo.OnRegionRecover(region);
        }
        if (recoverFlag) {
            Signal();
        }
    }

    bool IsEmpty() {
        PTScopedLock dataLock(mLock);
        for (LogstoreFeedBackQueueMapIterator iter = mLogstoreSenderQueueMap.begin();
             iter != mLogstoreSenderQueueMap.end();
             ++iter) {
            if (!iter->second.IsEmpty()) {
                return false;
            }
        }
        return true;
    }

    bool IsEmpty(const LogstoreFeedBackKey& key) {
        PTScopedLock dataLock(mLock);
        auto iter = mLogstoreSenderQueueMap.find(key);
        return iter == mLogstoreSenderQueueMap.end() || iter->second.IsEmpty();
    }

    void Delete(const LogstoreFeedBackKey& key) {
        PTScopedLock dataLock(mLock);
        auto iter = mLogstoreSenderQueueMap.find(key);
        if (iter != mLogstoreSenderQueueMap.end()) {
            mLogstoreSenderQueueMap.erase(iter);
        }
    }

    // do not clear real data, just for unit test
    void RemoveAll() {
        PTScopedLock dataLock(mLock);
        mLogstoreSenderQueueMap.clear();
        mSenderQueueBeginIndex = 0;
    }

    void Lock() { mLock.lock(); }

    void Unlock() { mLock.unlock(); }

    void SetUrgent() { mUrgentFlag = true; }

    void ResetUrgent() { mUrgentFlag = false; }

    LogstoreSenderStatistics GetSenderStatistics(const LogstoreFeedBackKey& key) {
        PTScopedLock dataLock(mLock);
        LogstoreFeedBackQueueMapIterator iter = mLogstoreSenderQueueMap.find(key);
        if (iter == mLogstoreSenderQueueMap.end()) {
            return LogstoreSenderStatistics();
        }
        return iter->second.GetSenderStatistics();
    }

    void GetStatus(int32_t& normalInvalidCount,
                   int32_t& normalInvalidSenderCount,
                   int32_t& normalTotalCount,
                   int32_t& eoInvalidCount,
                   int32_t& eoInvalidSenderCount,
                   int32_t& eoTotalCount) {
        int32_t curTime = time(NULL);
        PTScopedLock dataLock(mLock);
        for (LogstoreFeedBackQueueMapIterator iter = mLogstoreSenderQueueMap.begin();
             iter != mLogstoreSenderQueueMap.end();
             ++iter) {
            bool isExactlyOnceQueue = iter->second.GetQueueType() == QueueType::ExactlyOnce;
            auto& invalidCount = isExactlyOnceQueue ? eoInvalidCount : normalInvalidCount;
            auto& invalidSenderCount = isExactlyOnceQueue ? eoInvalidSenderCount : normalInvalidSenderCount;
            auto& totalCount = isExactlyOnceQueue ? eoTotalCount : normalTotalCount;

            ++totalCount;
            if (!iter->second.IsValid()) {
                ++invalidCount;
            }
            if (!iter->second.IsValidToSend(curTime)) {
                ++invalidSenderCount;
            }
        }
    }

    SingleLogstoreSenderManager<SenderQueueParam>* GetQueue(QueueKey key) {
        PTScopedLock dataLock(mLock);
        return &mLogstoreSenderQueueMap[key];
    }

protected:
    LogstoreFeedBackQueueMap mLogstoreSenderQueueMap;
    mutable PTMutex mLock;
    mutable TriggerEvent mTrigger;
    FeedbackInterface* mFeedBackObj;
    bool mUrgentFlag;
    size_t mSenderQueueBeginIndex;

private:
#ifdef APSARA_UNIT_TEST_MAIN
    friend class SenderUnittest;
    friend class ExactlyOnceReaderUnittest;

    void PrintStatus() {
        printf("================================\n");
        PTScopedLock dataLock(mLock);
        for (LogstoreFeedBackQueueMapIterator iter = mLogstoreSenderQueueMap.begin();
             iter != mLogstoreSenderQueueMap.end();
             ++iter) {
            SingleLogStoreManager& logstoreManager = iter->second;

            printf(" %d   %d   %s \n ",
                   (int32_t)iter->first,
                   (int32_t)logstoreManager.GetSize(),
                   logstoreManager.mSenderInfo.mRegion.c_str());
        }
        printf("================================\n");
    }

    LogstoreSenderInfo* GetSenderInfo(LogstoreFeedBackKey key) {
        PTScopedLock dataLock(mLock);
        LogstoreFeedBackQueueMapIterator iter = mLogstoreSenderQueueMap.find(key);
        if (iter == mLogstoreSenderQueueMap.end()) {
            return NULL;
        }
        return &(iter->second.mSenderInfo);
    }
#endif
};

} // namespace logtail
