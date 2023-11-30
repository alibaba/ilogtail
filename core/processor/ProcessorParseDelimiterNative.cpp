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

#include "processor/ProcessorParseDelimiterNative.h"

#include "common/ParamExtractor.h"
#include "models/LogEvent.h"
#include "monitor/MetricConstants.h"
#include "plugin/instance/ProcessorInstance.h"

namespace logtail {
const std::string ProcessorParseDelimiterNative::sName = "processor_parse_delimiter_native";

const std::string ProcessorParseDelimiterNative::s_mDiscardedFieldKey = "_";

bool ProcessorParseDelimiterNative::Init(const Json::Value& config) {
   std::string errorMsg;
   if (!GetMandatoryStringParam(config, "SourceKey", mSourceKey, errorMsg)) {
       PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
   }
   if (!GetMandatoryStringParam(config, "Separator", mSeparator, errorMsg)) {
       PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
   }
   if (mSeparator.size() > 4) {
       errorMsg = "Separator length should be no more than 4";
       PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
   }
   // Compatible with old logic.
   if (mSeparator == "\\t")
       mSeparator = '\t';
   std::string quoteStr;
   bool res = GetOptionalStringParam(config, "Quote", quoteStr, errorMsg);
   if (mSeparator.size() == 1) {
       if (!res || quoteStr.size() == 0) {
           PARAM_WARNING_DEFAULT(mContext->GetLogger(), errorMsg, mQuote, sName, mContext->GetConfigName());
       } else if (quoteStr.size() == 1) {
           mQuote = quoteStr[0];
       } else {
           errorMsg = "param Quote is not a single char";
           PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
       }
   } else if (mSeparator.size() > 1 && quoteStr.size() > 0) {
       errorMsg = "param Quote is not allowed when Separator length is more than 1";
       PARAM_WARNING_IGNORE(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
   }
   if (!mSeparator.empty())
       mSeparatorChar = mSeparator.data()[0];


   if (!GetMandatoryListParam(config, "Keys", mKeys, errorMsg)) {
       PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
   }
   for (auto key : mKeys) {
       if (key == mSourceKey) {
           mSourceKeyOverwritten = true;
       }
   }

   if (!GetOptionalBoolParam(config, "AllowingShortenedFields", mAllowingShortenedFields, errorMsg)) {
       PARAM_WARNING_DEFAULT(
           mContext->GetLogger(), errorMsg, mAllowingShortenedFields, sName, mContext->GetConfigName());
   }
   std::string overflowedFieldsTreatment;
   if (!GetOptionalStringParam(config, "OverflowedFieldsTreatment", overflowedFieldsTreatment, errorMsg)) {
       PARAM_WARNING_DEFAULT(
           mContext->GetLogger(), errorMsg, overflowedFieldsTreatment, sName, mContext->GetConfigName());
   }
   if (overflowedFieldsTreatment == "extend") {
       mOverflowedFieldsTreatment = OverflowedFieldsTreatment::EXTEND;
   } else if (overflowedFieldsTreatment == "keep") {
       mOverflowedFieldsTreatment = OverflowedFieldsTreatment::KEEP;
   } else if (overflowedFieldsTreatment == "discard") {
       mOverflowedFieldsTreatment = OverflowedFieldsTreatment::DISCARD;
   } else {
       PARAM_WARNING_DEFAULT(
           mContext->GetLogger(), errorMsg, overflowedFieldsTreatment, sName, mContext->GetConfigName());
   }
   mCommonParserOptions.Init(config, *mContext, sName);
   if (mCommonParserOptions.mRenamedSourceKey.empty()) {
       mCommonParserOptions.mRenamedSourceKey = mSourceKey;
   }

   mExtractPartialFields = mOverflowedFieldsTreatment == OverflowedFieldsTreatment::DISCARD;

   mDelimiterModeFsmParserPtr = new DelimiterModeFsmParser(mQuote, mSeparatorChar);
   mParseFailures = &(GetContext().GetProcessProfile().parseFailures);
   mLogGroupSize = &(GetContext().GetProcessProfile().logGroupSize);

   mProcParseInSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_IN_SIZE_BYTES);
   mProcParseOutSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_OUT_SIZE_BYTES);
   mProcDiscardRecordsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_DISCARD_RECORDS_TOTAL);
   mProcParseErrorTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_ERROR_TOTAL);
   return true;
}

void ProcessorParseDelimiterNative::Process(PipelineEventGroup& logGroup) {
   if (logGroup.GetEvents().empty()) {
       return;
   }
   const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
   EventsContainer& events = logGroup.MutableEvents();
   // works good normally. poor performance if most data need to be discarded.
   for (auto it = events.begin(); it != events.end();) {
       if (ProcessEvent(logPath, *it)) {
           ++it;
       } else {
           it = events.erase(it);
       }
   }
   return;
}

bool ProcessorParseDelimiterNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e) {
   if (!IsSupportedEvent(e)) {
       return true;
   }
   LogEvent& sourceEvent = e.Cast<LogEvent>();
   if (!sourceEvent.HasContent(mSourceKey)) {
       return true;
   }
   StringView buffer = sourceEvent.GetContent(mSourceKey);
   mProcParseInSizeBytes->Add(buffer.size());
   int32_t endIdx = buffer.size();
   if (endIdx == 0)
       return true;

   for (int32_t i = endIdx - 1; i >= 0; --i) {
       if (buffer.data()[i] == ' ' || '\r' == buffer.data()[i])
           endIdx = i;
       else
           break;
   }
   int32_t begIdx = 0;
   for (int32_t i = 0; i < endIdx; ++i) {
       if (buffer.data()[i] == ' ')
           begIdx = i + 1;
       else
           break;
   }
   if (begIdx >= endIdx)
       return true;
   size_t reserveSize
       = mOverflowedFieldsTreatment == OverflowedFieldsTreatment::EXTEND ? (mKeys.size() + 10) : (mKeys.size() + 1);
   std::vector<StringView> columnValues;
   std::vector<size_t> colBegIdxs;
   std::vector<size_t> colLens;
   bool parseSuccess = false;
   size_t parsedColCount = 0;
   bool useQuote = (mSeparator.size() == 1) && (mQuote != mSeparatorChar);
   if (mKeys.size() > 0) {
       if (useQuote) {
           columnValues.reserve(reserveSize);
           parseSuccess = mDelimiterModeFsmParserPtr->ParseDelimiterLine(buffer, begIdx, endIdx, columnValues);
           // handle auto extend
           if (!(mOverflowedFieldsTreatment == OverflowedFieldsTreatment::EXTEND)
               && columnValues.size() > mKeys.size()) {
               int requiredLen = 0;
               for (size_t i = mKeys.size(); i < columnValues.size(); ++i) {
                   requiredLen += 1 + columnValues[i].size();
               }
               StringBuffer sb = sourceEvent.GetSourceBuffer()->AllocateStringBuffer(requiredLen);
               char* extraFields = sb.data;
               for (size_t i = mKeys.size(); i < columnValues.size(); ++i) {
                   extraFields[0] = mSeparatorChar;
                   extraFields++;
                   memcpy(extraFields, columnValues[i].data(), columnValues[i].size());
                   extraFields += columnValues[i].size();
               }
               // remove extra fields
               columnValues.resize(mKeys.size());
               columnValues.push_back(StringView(sb.data, requiredLen));
           }
           parsedColCount = columnValues.size();
       } else {
           colBegIdxs.reserve(reserveSize);
           colLens.reserve(reserveSize);
           parseSuccess = SplitString(buffer.data(), begIdx, endIdx, colBegIdxs, colLens);
           parsedColCount = colBegIdxs.size();
       }

       if (parseSuccess) {
           if (parsedColCount <= 0 || (!mAllowingShortenedFields && parsedColCount < mKeys.size())) {
               LOG_WARNING(
                   sLogger,
                   ("parse delimiter log fail, keys count unmatch "
                    "columns count, parsed",
                    parsedColCount)("required", mKeys.size())("log", buffer)("project", GetContext().GetProjectName())(
                       "logstore", GetContext().GetLogstoreName())("file", logPath));
               GetContext().GetAlarm().SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                 std::string("keys count unmatch columns count :")
                                                     + ToString(parsedColCount) + ", required:"
                                                     + ToString(mKeys.size()) + ", logs:" + buffer.to_string(),
                                                 GetContext().GetProjectName(),
                                                 GetContext().GetLogstoreName(),
                                                 GetContext().GetRegion());
               mProcParseErrorTotal->Add(1);
               ++(*mParseFailures);
               parseSuccess = false;
           }
       } else {
           LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                  std::string("parse delimiter log fail")
                                                      + ", logs:" + buffer.to_string(),
                                                  GetContext().GetProjectName(),
                                                  GetContext().GetLogstoreName(),
                                                  GetContext().GetRegion());
           mProcParseErrorTotal->Add(1);
           ++(*mParseFailures);
           parseSuccess = false;
       }
   } else {
       LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                              "no column keys defined",
                                              GetContext().GetProjectName(),
                                              GetContext().GetLogstoreName(),
                                              GetContext().GetRegion());
       LOG_WARNING(sLogger,
                   ("parse delimiter log fail", "no column keys defined")("project", GetContext().GetProjectName())(
                       "logstore", GetContext().GetLogstoreName())("file", logPath));
       mProcParseErrorTotal->Add(1);
       ++(*mParseFailures);
       parseSuccess = false;
   }

   if (parseSuccess) {
       for (uint32_t idx = 0; idx < parsedColCount; idx++) {
           if (mKeys.size() > idx) {
               if (mExtractPartialFields && mKeys[idx] == s_mDiscardedFieldKey) {
                   continue;
               }
               AddLog(mKeys[idx],
                      useQuote ? columnValues[idx] : StringView(buffer.data() + colBegIdxs[idx], colLens[idx]),
                      sourceEvent);
           } else {
               if (mExtractPartialFields) {
                   continue;
               }
               std::string key = "__column" + ToString(idx) + "__";
               StringBuffer sb = sourceEvent.GetSourceBuffer()->CopyString(key);
               AddLog(StringView(sb.data, sb.size),
                      useQuote ? columnValues[idx] : StringView(buffer.data() + colBegIdxs[idx], colLens[idx]),
                      sourceEvent);
           }
       }
   }
   if (!parseSuccess || !mSourceKeyOverwritten) {
       sourceEvent.DelContent(mSourceKey);
   }
   if (mCommonParserOptions.ShouldAddRenamedSourceLog(parseSuccess)) {
       AddLog(mCommonParserOptions.mRenamedSourceKey, buffer, sourceEvent, false);
   }
   if (mCommonParserOptions.ShouldAddUnmatchLog(parseSuccess)) {
       AddLog(mCommonParserOptions.UNMATCH_LOG_KEY, buffer, sourceEvent, false);
   }
   if (mCommonParserOptions.ShouldEraseEvent(parseSuccess, sourceEvent)) {
       mProcDiscardRecordsTotal->Add(1);
       return false;
   }
   return true;
}

bool ProcessorParseDelimiterNative::SplitString(
   const char* buffer, int32_t begIdx, int32_t endIdx, std::vector<size_t>& colBegIdxs, std::vector<size_t>& colLens) {
   if (endIdx <= begIdx || mSeparator.size() == 0 || mKeys.size() == 0)
       return false;
   size_t size = endIdx - begIdx;
   size_t d_size = mSeparator.size();
   if (d_size == 0 || d_size > size) {
       colBegIdxs.push_back(begIdx);
       colLens.push_back(size);
       return true;
   }
   size_t pos = begIdx;
   size_t top = endIdx - d_size;
   while (pos <= top) {
       const char* pch = std::search(buffer + pos, buffer + endIdx, mSeparator.begin(), mSeparator.end());
       size_t pos2;
       // if not found, pos2 = endIdx
       if (pch == buffer + endIdx) {
           pos2 = endIdx;
       } else {
           pos2 = pch - buffer;
       }
       if (pos2 != pos) {
           colBegIdxs.push_back(pos);
           colLens.push_back(pos2 - pos);
       } else {
           colBegIdxs.push_back(pos);
           colLens.push_back(0);
       }
       if (pos2 == (size_t)endIdx)
           return true;
       pos = pos2 + d_size;
       if (colLens.size() >= mKeys.size() && !(mOverflowedFieldsTreatment == OverflowedFieldsTreatment::EXTEND)) {
           colBegIdxs.push_back(pos2);
           colLens.push_back(endIdx - pos2);
           return true;
       }
   }
   if (pos <= (size_t)endIdx) {
       colBegIdxs.push_back(pos);
       colLens.push_back(endIdx - pos);
   }
   return true;
}

void ProcessorParseDelimiterNative::AddLog(const StringView& key,
                                          const StringView& value,
                                          LogEvent& targetEvent,
                                          bool overwritten) {
   if (!overwritten && targetEvent.HasContent(key)) {
       return;
   }
   targetEvent.SetContentNoCopy(key, value);
   *mLogGroupSize += key.size() + value.size() + 5;
   mProcParseOutSizeBytes->Add(key.size() + value.size());
}

bool ProcessorParseDelimiterNative::IsSupportedEvent(const PipelineEventPtr& e) const {
   return e.Is<LogEvent>();
}

} // namespace logtail