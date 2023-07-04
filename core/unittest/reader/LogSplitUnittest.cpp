// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "unittest/Unittest.h"
#include "reader/CommonRegLogFileReader.h"
#include "reader/SourceBuffer.h"
#include "common/FileSystemUtil.h"

namespace logtail
{

static void CompareMultilineStringWithArray(std::string s, std::vector<StringView> strings) {
    size_t begPos = 0;
    size_t endPos = 0;
    int i = 0;
    while (endPos < s.size()) {
        if (s[endPos] == '\n') {
            APSARA_TEST_EQUAL_FATAL(s.substr(begPos, endPos - begPos), strings[i].to_string());
            begPos = endPos + 1;
            i++;
        }
        endPos++;
    }
}

class LogSplitUnittest : public ::testing::Test {
public:
    void TestLogSplitSingleLine();
    void TestLogSplitSingleLinePartNotMatch();
    void TestLogSplitSingleLinePartNotMatchNoDiscard();
    void TestLogSplitMultiLine();
    void TestLogSplitMultiLinePartNotMatch();
    void TestLogSplitMultiLinePartNotMatchNoDiscard();
    void TestLogSplitMultiLineAllNotmatch();
    void TestLogSplitMultiLineAllNotmatchNoDiscard();
    const std::string LOG_BEGIN_STRING = "Exception in thread \"main\" java.lang.NullPointerException";
    const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
    const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
    const std::string LOG_CONTINUE_REGEX = R"(\s*at.*)";
    const std::string LOG_END_STRING = "    ...23 more";
    const std::string LOG_END_REGEX = R"(\s*\.\.\..*)";
    const std::string LOG_UNMATCH = "unmatch log";
};

UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitSingleLine);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitSingleLinePartNotMatch);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitSingleLinePartNotMatchNoDiscard);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLine);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLinePartNotMatch);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLinePartNotMatchNoDiscard);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLineAllNotmatch);
UNIT_TEST_CASE(LogSplitUnittest, TestLogSplitMultiLineAllNotmatchNoDiscard);

void LogSplitUnittest::TestLogSplitSingleLine() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    std::string line1 = LOG_BEGIN_STRING + "first.";
    std::string line23 = "multiline1\nmultiline2";
    std::string line4 = LOG_BEGIN_STRING + "second.";
    std::string line56 = "multiline1\nmultiline2";
    std::string testLog = line1 + '\n' + line23 + '\n' + line4 + '\n' + line56;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(6UL, index.size());
    APSARA_TEST_EQUAL_FATAL(line1, index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(line4, index[3].to_string());
}

void LogSplitUnittest::TestLogSplitSingleLinePartNotMatch() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.mUnmatch = "discard";
    std::string line1 = "first.";
    std::string testLog = line1;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(0UL, index.size());
    APSARA_TEST_EQUAL_FATAL(1UL, discard.size());
}

void LogSplitUnittest::TestLogSplitSingleLinePartNotMatchNoDiscard() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false);
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.mUnmatch = "append";
    std::string line1 = "first.";
    std::string testLog = line1;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(1UL, index.size());
    APSARA_TEST_EQUAL_FATAL(line1, index[0].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLine() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.mUnmatch = "discard";
    std::string firstLog = LOG_BEGIN_STRING + "first.\nmultiline1\nmultiline2";
    std::string secondLog = LOG_BEGIN_STRING + "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(2UL, index.size());
    APSARA_TEST_EQUAL_FATAL(4UL, discard.size());
    APSARA_TEST_EQUAL_FATAL(firstLog.substr(0, firstLog.find('\n')), index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(secondLog.substr(0, secondLog.find('\n')), index[1].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLinePartNotMatch() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.mUnmatch = "discard";
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = LOG_BEGIN_STRING + "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(1UL, index.size());
    APSARA_TEST_EQUAL_FATAL(secondLog.substr(0, secondLog.find('\n')), index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(5UL, discard.size());
    CompareMultilineStringWithArray(firstLog, discard);
}

void LogSplitUnittest::TestLogSplitMultiLinePartNotMatchNoDiscard() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false);
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.mUnmatch = "append";
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = LOG_BEGIN_STRING + "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_TRUE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(2UL, index.size());
    APSARA_TEST_EQUAL_FATAL(firstLog, index[0].to_string());
    APSARA_TEST_EQUAL_FATAL(secondLog, index[1].to_string());
}

void LogSplitUnittest::TestLogSplitMultiLineAllNotmatch() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.mUnmatch = "discard";
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(0UL, index.size());
    APSARA_TEST_EQUAL_FATAL(6UL, discard.size());
    CompareMultilineStringWithArray(testLog, discard);
}

void LogSplitUnittest::TestLogSplitMultiLineAllNotmatchNoDiscard() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "", ENCODING_UTF8, false);
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.mUnmatch = "singleline";
    std::string firstLog = "first.\nmultiline1\nmultiline2";
    std::string secondLog = "second.\nmultiline1\nmultiline2";
    std::string testLog = firstLog + '\n' + secondLog;
    int32_t lineFeed = 0;
    std::vector<StringView> index;
    std::vector<StringView> discard;
    bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
    APSARA_TEST_FALSE_FATAL(splitSuccess);
    APSARA_TEST_EQUAL_FATAL(6UL, index.size());
    CompareMultilineStringWithArray(testLog, index);
}

class LogSplitDiscardUnmatchUnittest : public ::testing::Test {
public:
    void TestLogSplitWithBeginContinueEnd();
    void TestLogSplitWithBeginContinue();
    void TestLogSplitWithBeginEnd();
    void TestLogSplitWithBegin();
    void TestLogSplitWithContinueEnd();
    void TestLogSplitWithContinue();
    void TestLogSplitWithEnd();
    const std::string LOG_BEGIN_STRING = "Exception in thread \"main\" java.lang.NullPointerException";
    const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
    const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
    const std::string LOG_CONTINUE_REGEX = R"(\s*at.*)";
    const std::string LOG_END_STRING = "    ...23 more";
    const std::string LOG_END_REGEX = R"(\s*\.\.\..*)";
    const std::string LOG_UNMATCH = "unmatch log";
    const std::string UNMATCH_DISCARD = "discard";
    const std::string UNMATCH_SINGLELINE = "singleline";
    const std::string UNMATCH_APPEND = "append";
    const std::string UNMATCH_PREPEND = "prepend";
};

UNIT_TEST_CASE(LogSplitDiscardUnmatchUnittest, TestLogSplitWithBeginContinueEnd);
UNIT_TEST_CASE(LogSplitDiscardUnmatchUnittest, TestLogSplitWithBeginContinue);
UNIT_TEST_CASE(LogSplitDiscardUnmatchUnittest, TestLogSplitWithBeginEnd);
UNIT_TEST_CASE(LogSplitDiscardUnmatchUnittest, TestLogSplitWithBegin);
UNIT_TEST_CASE(LogSplitDiscardUnmatchUnittest, TestLogSplitWithContinueEnd);
UNIT_TEST_CASE(LogSplitDiscardUnmatchUnittest, TestLogSplitWithContinue);
UNIT_TEST_CASE(LogSplitDiscardUnmatchUnittest, TestLogSplitWithEnd);

void LogSplitDiscardUnmatchUnittest::TestLogSplitWithBeginContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_DISCARD);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[1].to_string());
    }
    { // case: incomplete log (begin, continue)
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(5UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(3UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
}

void LogSplitDiscardUnmatchUnittest::TestLogSplitWithBeginContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_DISCARD);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[1].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(3UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
}

void LogSplitDiscardUnmatchUnittest::TestLogSplitWithBeginEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_DISCARD);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[1].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(3UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
}

void LogSplitDiscardUnmatchUnittest::TestLogSplitWithBegin() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetUnmatch(UNMATCH_DISCARD);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
}

void LogSplitDiscardUnmatchUnittest::TestLogSplitWithContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_DISCARD);
    { // case: complete log
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[1].to_string());
    }
    { // case: incomplete log (continue)
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(4UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
}

void LogSplitDiscardUnmatchUnittest::TestLogSplitWithContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_DISCARD);
    { // case: complete log
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
}

void LogSplitDiscardUnmatchUnittest::TestLogSplitWithEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_DISCARD);
    { // case: complete log
        std::string expectMatch = LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, discard[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(0UL, index.size());
        APSARA_TEST_EQUAL_FATAL(2UL, discard.size());
        CompareMultilineStringWithArray(testLog, discard);
    }
}

class LogSplitSinglelineUnmatchUnittest : public ::testing::Test {
public:
    void TestLogSplitWithBeginContinueEnd();
    void TestLogSplitWithBeginContinue();
    void TestLogSplitWithBeginEnd();
    void TestLogSplitWithBegin();
    void TestLogSplitWithContinueEnd();
    void TestLogSplitWithContinue();
    void TestLogSplitWithEnd();
    const std::string LOG_BEGIN_STRING = "Exception in thread \"main\" java.lang.NullPointerException";
    const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
    const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
    const std::string LOG_CONTINUE_REGEX = R"(\s*at.*)";
    const std::string LOG_END_STRING = "    ...23 more";
    const std::string LOG_END_REGEX = R"(\s*\.\.\..*)";
    const std::string LOG_UNMATCH = "unmatch log";
    const std::string UNMATCH_DISCARD = "discard";
    const std::string UNMATCH_SINGLELINE = "singleline";
    const std::string UNMATCH_APPEND = "append";
    const std::string UNMATCH_PREPEND = "prepend";
};

UNIT_TEST_CASE(LogSplitSinglelineUnmatchUnittest, TestLogSplitWithBeginContinueEnd);
UNIT_TEST_CASE(LogSplitSinglelineUnmatchUnittest, TestLogSplitWithBeginContinue);
UNIT_TEST_CASE(LogSplitSinglelineUnmatchUnittest, TestLogSplitWithBeginEnd);
UNIT_TEST_CASE(LogSplitSinglelineUnmatchUnittest, TestLogSplitWithBegin);
UNIT_TEST_CASE(LogSplitSinglelineUnmatchUnittest, TestLogSplitWithContinueEnd);
UNIT_TEST_CASE(LogSplitSinglelineUnmatchUnittest, TestLogSplitWithContinue);
UNIT_TEST_CASE(LogSplitSinglelineUnmatchUnittest, TestLogSplitWithEnd);

void LogSplitSinglelineUnmatchUnittest::TestLogSplitWithBeginContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_SINGLELINE);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[2].to_string());
    }
    { // case: incomplete log (begin, continue)
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(5UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
}

void LogSplitSinglelineUnmatchUnittest::TestLogSplitWithBeginContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_SINGLELINE);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[2].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
}

void LogSplitSinglelineUnmatchUnittest::TestLogSplitWithBeginEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_SINGLELINE);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[2].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
}

void LogSplitSinglelineUnmatchUnittest::TestLogSplitWithBegin() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetUnmatch(UNMATCH_SINGLELINE);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[2].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
}

void LogSplitSinglelineUnmatchUnittest::TestLogSplitWithContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_SINGLELINE);
    { // case: complete log
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[2].to_string());
    }
    { // case: incomplete log (continue)
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(4UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
}

void LogSplitSinglelineUnmatchUnittest::TestLogSplitWithContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_SINGLELINE);
    { // case: complete log
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[2].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
}

void LogSplitSinglelineUnmatchUnittest::TestLogSplitWithEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_SINGLELINE);
    { // case: complete log
        std::string expectMatch = LOG_END_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(3UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[2].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        CompareMultilineStringWithArray(testLog, index);
    }
}

class LogSplitAppendUnmatchUnittest : public ::testing::Test {
public:
    void TestLogSplitWithBeginContinueEnd();
    void TestLogSplitWithBeginContinue();
    void TestLogSplitWithBeginEnd();
    void TestLogSplitWithBegin();
    void TestLogSplitWithContinueEnd();
    void TestLogSplitWithContinue();
    void TestLogSplitWithEnd();
    const std::string LOG_BEGIN_STRING = "Exception in thread \"main\" java.lang.NullPointerException";
    const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
    const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
    const std::string LOG_CONTINUE_REGEX = R"(\s*at.*)";
    const std::string LOG_END_STRING = "    ...23 more";
    const std::string LOG_END_REGEX = R"(\s*\.\.\..*)";
    const std::string LOG_UNMATCH = "unmatch log";
    const std::string UNMATCH_DISCARD = "discard";
    const std::string UNMATCH_SINGLELINE = "singleline";
    const std::string UNMATCH_APPEND = "append";
    const std::string UNMATCH_PREPEND = "prepend";
};

UNIT_TEST_CASE(LogSplitAppendUnmatchUnittest, TestLogSplitWithBeginContinueEnd);
UNIT_TEST_CASE(LogSplitAppendUnmatchUnittest, TestLogSplitWithBeginContinue);
UNIT_TEST_CASE(LogSplitAppendUnmatchUnittest, TestLogSplitWithBeginEnd);
UNIT_TEST_CASE(LogSplitAppendUnmatchUnittest, TestLogSplitWithBegin);
UNIT_TEST_CASE(LogSplitAppendUnmatchUnittest, TestLogSplitWithContinueEnd);
UNIT_TEST_CASE(LogSplitAppendUnmatchUnittest, TestLogSplitWithContinue);
UNIT_TEST_CASE(LogSplitAppendUnmatchUnittest, TestLogSplitWithEnd);

void LogSplitAppendUnmatchUnittest::TestLogSplitWithBeginContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_APPEND);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + '\n' + LOG_UNMATCH;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
    }
    { // case: incomplete log (begin, continue)
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitAppendUnmatchUnittest::TestLogSplitWithBeginContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_APPEND);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n' + LOG_UNMATCH;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitAppendUnmatchUnittest::TestLogSplitWithBeginEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_APPEND);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_END_STRING + '\n' + LOG_UNMATCH;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitAppendUnmatchUnittest::TestLogSplitWithBegin() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetUnmatch(UNMATCH_APPEND);
    { // case: complete log
        std::string expectMatch = LOG_BEGIN_STRING + '\n' + LOG_UNMATCH;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitAppendUnmatchUnittest::TestLogSplitWithContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_APPEND);
    { // case: complete log
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING + '\n' + LOG_UNMATCH;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
    }
    { // case: incomplete log (continue)
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitAppendUnmatchUnittest::TestLogSplitWithContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_APPEND);
    { // case: complete log
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + '\n' + LOG_UNMATCH;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitAppendUnmatchUnittest::TestLogSplitWithEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_APPEND);
    { // case: complete log
        std::string expectMatch = LOG_END_STRING + '\n' + LOG_UNMATCH;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

class LogSplitPrependUnmatchUnittest : public ::testing::Test {
public:
    void TestLogSplitWithBeginContinueEnd();
    void TestLogSplitWithBeginContinue();
    void TestLogSplitWithBeginEnd();
    void TestLogSplitWithBegin();
    void TestLogSplitWithContinueEnd();
    void TestLogSplitWithContinue();
    void TestLogSplitWithEnd();
    const std::string LOG_BEGIN_STRING = "Exception in thread \"main\" java.lang.NullPointerException";
    const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
    const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
    const std::string LOG_CONTINUE_REGEX = R"(\s*at.*)";
    const std::string LOG_END_STRING = "    ...23 more";
    const std::string LOG_END_REGEX = R"(\s*\.\.\..*)";
    const std::string LOG_UNMATCH = "unmatch log";
    const std::string UNMATCH_DISCARD = "discard";
    const std::string UNMATCH_SINGLELINE = "singleline";
    const std::string UNMATCH_APPEND = "append";
    const std::string UNMATCH_PREPEND = "prepend";
};

UNIT_TEST_CASE(LogSplitPrependUnmatchUnittest, TestLogSplitWithBeginContinueEnd);
UNIT_TEST_CASE(LogSplitPrependUnmatchUnittest, TestLogSplitWithBeginContinue);
UNIT_TEST_CASE(LogSplitPrependUnmatchUnittest, TestLogSplitWithBeginEnd);
UNIT_TEST_CASE(LogSplitPrependUnmatchUnittest, TestLogSplitWithBegin);
UNIT_TEST_CASE(LogSplitPrependUnmatchUnittest, TestLogSplitWithContinueEnd);
UNIT_TEST_CASE(LogSplitPrependUnmatchUnittest, TestLogSplitWithContinue);
UNIT_TEST_CASE(LogSplitPrependUnmatchUnittest, TestLogSplitWithEnd);

void LogSplitPrependUnmatchUnittest::TestLogSplitWithBeginContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_PREPEND);
    { // case: complete log
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING;
        std::string testLog = expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[1].to_string());
    }
    { // case: incomplete log (begin, continue)
        std::string expectMatch = LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitPrependUnmatchUnittest::TestLogSplitWithBeginContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_PREPEND);
    { // case: complete log
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_BEGIN_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[1].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitPrependUnmatchUnittest::TestLogSplitWithBeginEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_PREPEND);
    { // case: complete log
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_BEGIN_STRING + "\n" + LOG_END_STRING;
        std::string testLog = expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[1].to_string());
    }
    { // case: incomplete log (begin)
        std::string expectMatch = LOG_BEGIN_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitPrependUnmatchUnittest::TestLogSplitWithBegin() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogBeginRegex(LOG_BEGIN_REGEX);
    logFileReader.SetUnmatch(UNMATCH_PREPEND);
    { // case: complete log
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_BEGIN_STRING;
        std::string testLog = expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitPrependUnmatchUnittest::TestLogSplitWithContinueEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_PREPEND);
    { // case: complete log
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_END_STRING;
        std::string testLog = expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[1].to_string());
    }
    { // case: incomplete log (continue)
        std::string expectMatch = LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = LOG_UNMATCH + "\n" + expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitPrependUnmatchUnittest::TestLogSplitWithContinue() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogContinueRegex(LOG_CONTINUE_REGEX);
    logFileReader.SetUnmatch(UNMATCH_PREPEND);
    { // case: complete log
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_CONTINUE_STRING + "\n" + LOG_CONTINUE_STRING;
        std::string testLog = expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

void LogSplitPrependUnmatchUnittest::TestLogSplitWithEnd() {
    CommonRegLogFileReader logFileReader(
        "project", "logstore", "dir", "file", INT32_FLAG(default_tail_limit_kb), "", "", "");
    logFileReader.SetLogEndRegex(LOG_END_REGEX);
    logFileReader.SetUnmatch(UNMATCH_PREPEND);
    { // case: complete log
        std::string expectMatch = LOG_UNMATCH + "\n" + LOG_END_STRING;
        std::string testLog = expectMatch + '\n' + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_TRUE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(2UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(expectMatch, index[0].to_string());
        APSARA_TEST_EQUAL_FATAL(LOG_UNMATCH, index[1].to_string());
    }
    { // case: no match log
        std::string expectMatch = "";
        std::string testLog = LOG_UNMATCH + "\n" + LOG_UNMATCH;
        int32_t lineFeed = 0;
        std::vector<StringView> index;
        std::vector<StringView> discard;
        bool splitSuccess = logFileReader.LogSplit(testLog.data(), testLog.size(), lineFeed, index, discard);
        APSARA_TEST_FALSE_FATAL(splitSuccess);
        APSARA_TEST_EQUAL_FATAL(1UL, index.size());
        APSARA_TEST_EQUAL_FATAL(0UL, discard.size());
        APSARA_TEST_EQUAL_FATAL(testLog, index[0].to_string());
    }
}

} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}