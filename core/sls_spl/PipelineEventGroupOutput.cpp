#include "sls_spl/PipelineEventGroupOutput.h"
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "logger/Logger.h"
#include "sls_spl/SplConstants.h"



namespace apsara::sls::spl {

void PipelineEventGroupOutput::setHeader(const IOHeader& header, std::string& err) {
    mRowSize = header.rowSize;
    mColumnNames = header.columnNames;
    for (int32_t i=0; i<header.columnNames.size(); i++) {

        auto field = header.columnNames[i].ToString();

        //LOG_INFO(sLogger, ("columeName", field));

        auto length = field.length();

        if (length == LENGTH_FIELD_TIMESTAMP && field == FIELD_TIMESTAMP) {
            mTimeIdx = i;
        } else if (length == LENGTH_FIELD_TIMESTAMP_NANOSECOND && field == FIELD_TIMESTAMP_NANOSECOND) {
            mTimeNSIdx = i;
        } else if (length >= LENGTH_FIELD_PREFIX_TAG && 
                field.compare(0, LENGTH_FIELD_PREFIX_TAG, FIELD_PREFIX_TAG) == 0) { // __tag__:*
            mTagsIdxs.push_back(i);
        } else { // content
            mContentsIdxs.push_back(i);
        }
    }
    for (auto& constCol : header.constCols) {
        mConstColumns.emplace(constCol.first, constCol.second.ToString());
        //LOG_INFO(sLogger, ("constCol key", constCol.first)("constCol value", constCol.second.ToString()));
    }
}

void PipelineEventGroupOutput::addRow(
    const std::vector<SplStringPiece>& row,
    const uint32_t time,
    const uint32_t timeNsPart,
    const ErrorKV& errorKV,
    std::string& error) {
    
    std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(mLogGroup->GetSourceBuffer());

    //LOG_INFO(sLogger, ("row.size()", row.size()));

    std::string tagStr = "";
    for (const auto& idxTag : mTagsIdxs) { 
        tagStr += StringView(row[idxTag].mPtr, row[idxTag].mLen).to_string();
        //LOG_INFO(sLogger, ("tag key", StringView(mColumnNames[idxTag].mPtr, mColumnNames[idxTag].mLen))("tag value", StringView(row[idxTag].mPtr, row[idxTag].mLen)));
    }

    int32_t logGroupKeyIdx = -1;
    auto it = mLogGroupKeyIdxs.find(tagStr);
    if (it != mLogGroupKeyIdxs.end()) {
        logGroupKeyIdx = it->second;
    } else {
        mLogGroupList->emplace_back(mLogGroup->GetSourceBuffer());
        mLogGroupList->back().SetAllMetadata(mLogGroup->MutableAllMetadata());
        logGroupKeyIdx = mLogGroupList->size() - 1;
        mLogGroupKeyIdxs.emplace(tagStr, logGroupKeyIdx);
    }

    targetEvent->SetTimestamp(time, timeNsPart);

    for (const auto& idxContent : mContentsIdxs) {
        if (!row[idxContent].hasValue()) {
            continue;
        }
        if (row[idxContent].isNull()) {
            targetEvent->SetContent(StringView(mColumnNames[idxContent].mPtr, mColumnNames[idxContent].mLen), StringView(NULL_STR.c_str(), NULL_STR.length()));
        } else {
            targetEvent->SetContent(StringView(mColumnNames[idxContent].mPtr, mColumnNames[idxContent].mLen), StringView(row[idxContent].mPtr, row[idxContent].mLen));
        }
        //LOG_INFO(sLogger, ("content key", StringView(mColumnNames[idxContent].mPtr, mColumnNames[idxContent].mLen))("content value", StringView(row[idxContent].mPtr, row[idxContent].mLen)));
    }

    for (const auto& idxTag : mTagsIdxs) { 
        mLogGroupList->at(logGroupKeyIdx).SetTag(StringView(mColumnNames[idxTag].mPtr, mColumnNames[idxTag].mLen), StringView(row[idxTag].mPtr, row[idxTag].mLen));
    }
    
    mLogGroupList->at(logGroupKeyIdx).AddEvent(std::move(targetEvent));
    if (!errorKV.second.empty()) {
        LOG_INFO(sLogger, ("__error__", errorKV.second));
    }
}


void PipelineEventGroupOutput::finish(std::string& error) {

}

}  // namespace apsara::sls::spl