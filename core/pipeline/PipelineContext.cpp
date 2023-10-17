#include "pipeline/PipelineContext.h"

#include "flusher/FlusherSLS.h"

using namespace std;

namespace logtail {
const string PipelineContext::sEmptyString = "";

const string& PipelineContext::GetProjectName() const {
    // return mSLSInfo ? mSLSInfo->mProject : sEmptyString;
    return mProjectName;
}

const string& PipelineContext::GetLogstoreName() const {
    // return mSLSInfo ? mSLSInfo->mLogstore : sEmptyString;
    return mLogstoreName;
}

const string& PipelineContext::GetRegion() const {
    // return mSLSInfo ? mSLSInfo->mRegion : sEmptyString;
    return mRegion;
}

LogstoreFeedBackKey PipelineContext::GetLogstoreKey() const {
    if (mSLSInfo) {
        return mSLSInfo->GetLogstoreKey();
    }
    static LogstoreFeedBackKey key = GenerateLogstoreFeedBackKey(sEmptyString, sEmptyString);
    return key;
}
} // namespace logtail
