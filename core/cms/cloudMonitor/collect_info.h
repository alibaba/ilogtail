#ifndef ARGUS_COLLECT_INFO_H
#define ARGUS_COLLECT_INFO_H

#include <string>
#include <utility>

#include "common/Singleton.h"
#include "common/InstanceLock.h"

namespace cloudMonitor {
    class CollectInfo {
    public:
        CollectInfo() = default;
        ~CollectInfo() = default;

        static std::string GetVersion();
        static std::string GetCompileTime();

        inline void SetLastCommitCost(double lastCommitCost) {
            mLastCommitCost = lastCommitCost;
        }

        inline double GetLastCommitCost() const {
            return mLastCommitCost;
        }

        inline void SetLastCommitCode(int lastCommitCode) {
            mLastCommitCode = lastCommitCode;
        }

        inline int GetLastCommitCode() const {
            return mLastCommitCode;
        }

        inline void SetLastCommitMsg(std::string lastCommitMsg) {
            mLastCommitMsg = std::move(lastCommitMsg);
        }

        inline std::string GetLastCommitMsg() {
            return mLastCommitMsg;
        }

        inline void SetPutMetricFailCount(size_t putMetricFailCount) {
            mPutMetricFailCount = putMetricFailCount;
        }

        inline size_t GetPutMetricFailCount() const {
            return mPutMetricFailCount;
        }

        inline void SetPutMetricFailPerMinute(double putMetricFailPerMinute) {
            mPutMetricFailPerMinute = putMetricFailPerMinute;
        }

        inline double GetPutMetricFailPerMinute() const {
            return mPutMetricFailPerMinute;
        }

        inline void SetPutMetricSuccCount(size_t putMetricSuccCount) {
            mPutMetricSuccCount = putMetricSuccCount;
        }

        inline size_t GetPutMetricSuccCount() const {
            return mPutMetricSuccCount;
        }

        inline void SetPullConfigFailCount(size_t pullConfigFailCount) {
            mPullConfigFailCount = pullConfigFailCount;
        }

        inline size_t GetPullConfigFailCount() const {
            return mPullConfigFailCount;
        }

        inline void SetPullConfigFailPerMinute(double pullConfigFailPerMinute) {
            mPullConfigFailPerMinute = pullConfigFailPerMinute;
        }

        inline double GetPullConfigFailPerMinute() const {
            return mPullConfigFailPerMinute;
        }

        inline void SetPullConfigSuccCount(size_t pullConfigSuccCount) {
            mPullConfigSuccCount = pullConfigSuccCount;
        }

        inline size_t GetPullConfigSuccCount() const {
            return mPullConfigSuccCount;
        }

    private:
        double mLastCommitCost = 0;
        int mLastCommitCode = 0;
        std::string mLastCommitMsg;

        size_t mPutMetricFailCount = 0;
        double mPutMetricFailPerMinute = 0;
        size_t mPutMetricSuccCount = 0;

        size_t mPullConfigFailCount = 0;
        double mPullConfigFailPerMinute = 0;
        size_t mPullConfigSuccCount = 0;

    };

    typedef common::Singleton<CollectInfo> SingletonCollectInfo;
}
#endif