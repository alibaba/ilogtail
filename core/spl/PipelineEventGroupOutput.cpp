#include "spl/PipelineEventGroupOutput.h"
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "logger/Logger.h"
#include "common/HashUtil.h"
#include "spl/SplConstants.h"
#include <boost/container_hash/hash.hpp>



namespace apsara::sls::spl {

void PipelineEventGroupOutput::setHeader(const IOHeader& header, std::string& err) {
    mIOHeader = &header;
    for (int32_t i=0; i<header.columnNames.size(); i++) {
        auto field = header.columnNames[i].ToString();
        auto length = field.length();
        if (length >= LENGTH_FIELD_PREFIX_TAG && 
                field.compare(0, LENGTH_FIELD_PREFIX_TAG, FIELD_PREFIX_TAG) == 0) { // __tag__:*
            mTagsIdxs.push_back(i);
            mColumns.emplace_back(mLogGroup->GetSourceBuffer()->CopyString(StringView(mIOHeader->columnNames[i].mPtr, mIOHeader->columnNames[i].mLen).substr(LENGTH_FIELD_PREFIX_TAG)));
        } else { // content
            mContentsIdxs.push_back(i);
            mColumns.emplace_back(mLogGroup->GetSourceBuffer()->CopyString(header.columnNames[i].mPtr, header.columnNames[i].mLen));
        }
    }
    //for (auto& columName : mIOHeader->columnNames) {
    //    mColumns.emplace_back(mLogGroup->GetSourceBuffer()->CopyString(columName.mPtr, columName.mLen));
    //}
}

void PipelineEventGroupOutput::addRow(
    const std::vector<SplStringPiece>& row,
    const uint32_t time,
    const uint32_t timeNsPart,
    const ErrorKV& errorKV,
    std::string& error) {
    std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(mLogGroup->GetSourceBuffer());

    size_t tagStrHash = 0;
    for (const auto& idxTag : mTagsIdxs) { 
        boost::hash_combine(tagStrHash, mIOHeader->columnNames[idxTag].hash());
        boost::hash_combine(tagStrHash, row[idxTag].hash());
    }

    int32_t logGroupKeyIdx = -1;
    if (tagStrHash != lastTagStrHash) {
        mLogGroupList->emplace_back(mLogGroup->GetSourceBuffer());
        mLogGroupList->back().SetAllMetadata(mLogGroup->GetAllMetadata());
    }
    logGroupKeyIdx = mLogGroupList->size() - 1;
    lastTagStrHash = tagStrHash;

    targetEvent->SetTimestamp(time, timeNsPart);

    for (const auto& idxContent : mContentsIdxs) {
        if (!row[idxContent].hasValue()) {
            continue;
        }
        if (row[idxContent].isNull()) {
            targetEvent->SetContent(mColumns[idxContent], StringView(NULL_STR.c_str(), NULL_STR.length()));
        } else {
            targetEvent->SetContent(mColumns[idxContent], StringView(row[idxContent].mPtr, row[idxContent].mLen));
        }
    }

    for (const auto& idxTag : mTagsIdxs) {
        mLogGroupList->at(logGroupKeyIdx).SetTag(mColumns[idxTag], StringView(row[idxTag].mPtr, row[idxTag].mLen));
    }
    
    mLogGroupList->at(logGroupKeyIdx).AddEvent(std::move(targetEvent));
    if (!errorKV.second.empty()) {
        LOG_ERROR(sLogger, ("__error__", errorKV.second));
    }
    mRowCount ++;
}

bool PipelineEventGroupOutput::isColumnar() {
    return false;
}

}  // namespace apsara::sls::spl