// Copyright 2022 iLogtail Authors
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

#include <iostream>
#include <string>

#include "inner_parser.h"
#include "type.h"

#define CHECK_PARTIAL \
    do { \
        if (!this->OK()) { \
            return ParseResult_Partial; \
        } \
    } while (0)


namespace logtail {
StringPiece RedisParser::readUtilNewLine() {
    const char* s = readChar();
    const char* ch = s;
    const char* lastCh = s;
    int counter = 1;
    while (true) {
        ch++;
        counter++;
        positionCommit(1);
        if (isParseFail) {
            StringPiece empty;
            return empty;
        }

        if ((*lastCh) == '\r' && (*ch) == '\n') {
            break;
        }
        lastCh = ch;
    }
    StringPiece val(s, counter - 2);
    return val;
}

ParseResult RedisParser::readData(std::vector<StringPiece>& data) {
    const char* ch = readChar();
    switch (*ch) {
        case kErrorFlag:
            redisData.isError = true;
        case kNumberFlag:
        case kSimpleStringFlag: // + 字符串 \r\n
        {
            auto s = readUtilNewLine();
            CHECK_PARTIAL;
            data.push_back(s);
            break;
        }
        case kBulkStringFlag: // $ 字符串的长度 \r\n 字符串 \r\n
        {
            auto s = readUtilNewLine();
            CHECK_PARTIAL;
            auto len = std::stoi(std::string(s.data(), s.size()));
            if (len > getLeftSize()) {
                return ParseResult_Partial;
            }
            auto s2 = readUtilNewLine();
            CHECK_PARTIAL;
            data.push_back(s2);
            break;
        }
        case kArrayFlag: // * 数组元素个数 \r\n 其他所有类型 (结尾不需要\r\n)
        {
            auto s = readUtilNewLine();
            CHECK_PARTIAL;
            auto cnt = std::stoi(std::string(s.data(), s.size()));
            for (int i = 0; i < cnt; i++) {
                auto res = readData(data);
                if (res != ParseResult_OK) {
                    return res;
                }
            }
            break;
        }
        default:
            return ParseResult_Fail;
    }
    return ParseResult_OK;
}

ParseResult RedisParser::parse() {
    return readData(redisData.data);
}

void RedisParser::print() {
    for (auto v : redisData.data) {
        std::cout << std::string(v.data(), v.size()) << " ";
    }

    std::cout << std::endl;
}

} // end of namespace logtail