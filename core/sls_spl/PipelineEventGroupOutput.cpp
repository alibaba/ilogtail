#include "sls_spl/PipelineEventGroupOutput.h"
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "logger/Logger.h"
#include "common/HashUtil.h"
#include "sls_spl/SplConstants.h"



namespace apsara::sls::spl {

void PipelineEventGroupOutput::setHeader(const IOHeader& header, std::string& err) {
    mIOHeader = &header;
    for (int32_t i=0; i<header.columnNames.size(); i++) {
        auto field = header.columnNames[i].ToString();
        LOG_DEBUG(sLogger, ("columeName", field));
        auto length = field.length();
        if (length >= LENGTH_FIELD_PREFIX_TAG && 
                field.compare(0, LENGTH_FIELD_PREFIX_TAG, FIELD_PREFIX_TAG) == 0) { // __tag__:*
            mTagsIdxs.push_back(i);
        } else { // content
            mContentsIdxs.push_back(i);
        }
    }

}

void PipelineEventGroupOutput::addRow(
    const std::vector<SplStringPiece>& row,
    const uint32_t time,
    const uint32_t timeNsPart,
    const ErrorKV& errorKV,
    std::string& error) {
    std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(mLogGroup->GetSourceBuffer());
    std::ostringstream oss;
    for (const auto& idxTag : mTagsIdxs) { 
        oss << mIOHeader->columnNames[idxTag] << row[idxTag];
        LOG_DEBUG(sLogger, ("tag key", StringView(mIOHeader->columnNames[idxTag].mPtr, mIOHeader->columnNames[idxTag].mLen))("tag value", StringView(row[idxTag].mPtr, row[idxTag].mLen)));
    }
    int64_t tagStrHash = HashString(oss.str());
    int32_t logGroupKeyIdx = -1;
    auto it = mLogGroupKeyIdxs.find(tagStrHash);
    if (it != mLogGroupKeyIdxs.end()) {
        logGroupKeyIdx = it->second;
    } else {
        mLogGroupList->emplace_back(mLogGroup->GetSourceBuffer());
        mLogGroupList->back().SetAllMetadata(mLogGroup->GetAllMetadata());
        logGroupKeyIdx = mLogGroupList->size() - 1;
        mLogGroupKeyIdxs.emplace(tagStrHash, logGroupKeyIdx);
    }

    targetEvent->SetTimestamp(time, timeNsPart);

    for (const auto& idxContent : mContentsIdxs) {
        if (!row[idxContent].hasValue()) {
            continue;
        }
        if (row[idxContent].isNull()) {
            targetEvent->SetContent(StringView(mIOHeader->columnNames[idxContent].mPtr, mIOHeader->columnNames[idxContent].mLen), StringView(NULL_STR.c_str(), NULL_STR.length()));
        } else {
            targetEvent->SetContent(StringView(mIOHeader->columnNames[idxContent].mPtr, mIOHeader->columnNames[idxContent].mLen), StringView(row[idxContent].mPtr, row[idxContent].mLen));
        }
        LOG_DEBUG(sLogger, ("content key", StringView(mIOHeader->columnNames[idxContent].mPtr, mIOHeader->columnNames[idxContent].mLen))("content value", StringView(row[idxContent].mPtr, row[idxContent].mLen)));
    }

    for (const auto& idxTag : mTagsIdxs) {
        mLogGroupList->at(logGroupKeyIdx).SetTag(StringView(mIOHeader->columnNames[idxTag].mPtr, mIOHeader->columnNames[idxTag].mLen).substr(LENGTH_FIELD_PREFIX_TAG), StringView(row[idxTag].mPtr, row[idxTag].mLen));
    }
    
    mLogGroupList->at(logGroupKeyIdx).AddEvent(std::move(targetEvent));
    if (!errorKV.second.empty()) {
        LOG_ERROR(sLogger, ("__error__", errorKV.second));
    }
    mRowCount ++;
}


void PipelineEventGroupOutput::finish(std::string& error) {
}

}  // namespace apsara::sls::spl