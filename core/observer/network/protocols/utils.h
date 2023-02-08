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

#include <endian.h>
#include <vector>
#include <iostream>
#include <arpa/inet.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <map>
#include <cstring>
#include "StringPiece.h"

namespace logtail {
inline void hexstring_to_bin(std::string s, std::vector<uint8_t>& dest) {
    auto p = s.data();
    auto end = p + s.length();

    while (p < end) {
        const char hexbytestr[] = {*p, *(p + 1), 0};
        uint8_t val = strtol(hexbytestr, NULL, 16);
        dest.push_back(val);
        p += 2;
    }
}

inline std::string charToHexString(const char* payload, const int32_t len, const int32_t limit) {
    std::stringstream ioss;
    std::string hexstring;
    for (int i = 0; i < len && i < limit; i++) {
        ioss << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)payload[i];
    }
    ioss >> hexstring;
    return hexstring;
}

class ProtoParser {
public:
    ProtoParser(const char* payload, const size_t pktSize, bool bigByteOrder)
        : currPostion(0), isParseFail(false), payload(payload), pktSize(pktSize), bigByteOrder(bigByteOrder) {
        parserFailMsg = std::string();
        //  needByteOrderCovert = isCPUBigByteOrder() != bigByteOrder;
    }

    ~ProtoParser() {}

    bool OK() { return !isParseFail; }

    uint64_t readUint64(bool commit = true) {
        if (this->currPostion + 8 > this->pktSize) {
            this->setParseFail("unexcepted eof");
            return 0;
        }
        uint64_t data = *((uint64_t*)(payload + this->currPostion));
        if (commit) {
            this->currPostion += 8;
        }

        if (bigByteOrder) {
            return be64toh(data);
        }
        return le64toh(data);
    }

    uint32_t readUint32(bool commit = true) {
        if (this->currPostion + 4 > this->pktSize) {
            this->setParseFail("unexcepted eof");
            return 0;
        }
        uint32_t data = *((uint32_t*)(payload + this->currPostion));
        if (commit) {
            this->currPostion += 4;
        }

        if (bigByteOrder) {
            return be32toh(data);
        }
        return le32toh(data);
    }

    uint16_t readUint16(bool commit = true) {
        if (this->currPostion + 2 > this->pktSize) {
            this->setParseFail("unexcepted eof");
            return 0;
        }
        uint16_t data = *((uint16_t*)(payload + this->currPostion));
        if (commit) {
            this->currPostion += 2;
        }

        if (bigByteOrder) {
            return be16toh(data);
        }
        return le16toh(data);
    }

    uint8_t readUint8(bool commit = true) {
        if (this->currPostion + 1 > this->pktSize) {
            this->setParseFail("unexcepted eof");
            return 0;
        }
        uint8_t data = *((uint8_t*)(payload + this->currPostion));
        if (commit) {
            this->currPostion += 1;
        }
        return data;
    }

    const char* readChar(bool commit = true) {
        if (this->currPostion + 1 > this->pktSize) {
            this->setParseFail("unexcepted eof");
            return 0;
        }
        const char* data = payload + this->currPostion;
        if (commit) {
            this->currPostion += 1;
        }
        return data;
    }

    StringPiece readUntil(char flag, bool commit = true) {
        if (this->currPostion + 1 > this->pktSize) {
            this->setParseFail("unexcepted eof");
            return {};
        }
        size_t len = 0;
        uint32_t pos = currPostion;
        while (pos + len + 1 <= this->pktSize && *(payload + pos + len) != flag) {
            len++;
        }
        if (commit && pos + 1 <= this->pktSize) {
            this->currPostion = pos + len + 1;
        }
        return {payload + pos, len};
    }


    void positionCommit(uint64_t step) {
        if (this->currPostion + step > this->pktSize) {
            this->setParseFail("unexcepted eof");
            return;
        }
        this->currPostion += step;
    }

    void positionBack(uint64_t step) {
        if (this->currPostion - step < 0) {
            this->setParseFail("unexcepted eof");
            return;
        }
        this->currPostion -= step;
    }


    bool isNextEof() {
        if (this->currPostion + 1 >= this->pktSize) {
            return true;
        } else {
            return false;
        }
    }
    template <uint8_t Len>
    int64_t readVarintCore(bool commit = true) {
        static const uint8_t kFirstMask = 0x80;
        static const uint8_t kLastMask = 0x7f;
        static const uint8_t kLen = 7;
        int64_t value = 0;
        uint32_t offset = 0;
        for (int i = 0; i < Len; i += kLen) {
            uint8_t b = readUint8(true);
            ++offset;
            if (!this->OK()) {
                this->positionBack(offset);
                return -1;
            }
            if (!(b & kFirstMask)) {
                value |= b << i;
                if (!commit) {
                    this->positionBack(offset);
                }
                return value;
            }
            value |= ((b & kLastMask) << i);
        }
        this->positionBack(offset);
        return -1;
    }

    int64_t readVarInt32(bool commit = true) { return readVarintCore<35>(commit); }

    int64_t readVarInt64(bool commit = true) { return readVarintCore<64>(commit); }

    const char* head() { return this->payload + this->currPostion; }

    uint32_t getPosition() { return currPostion; }

    int32_t getLeftSize() { return pktSize - this->currPostion; }

protected:
    void setParseFail(const std::string& failMsg) {
        this->isParseFail = true;
        this->parserFailMsg = failMsg;
        throw std::runtime_error("unexcepted eof");
    }

protected:
    // common fields

    uint32_t currPostion;

    std::string parserFailMsg;

    bool isParseFail;

    const char* payload;

    const size_t pktSize;

    // bool needByteOrderCovert;
    bool bigByteOrder;
};

} // end of namespace logtail