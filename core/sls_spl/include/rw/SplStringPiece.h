#pragma once

#include <re2/stringpiece.h>
#include <cstring>
#include <iostream>
#include <string>

namespace apsara::sls::spl {

struct SplStringPiece {
    const char* mPtr;
    int32_t mLen;
    bool mHasValue;
    bool mIsNull;

    SplStringPiece()
        : mPtr(nullptr), mLen(0), mHasValue(true), mIsNull(false) {
    }
    // for value not exist
    SplStringPiece(bool hasValue, bool isNull = false)
        : mPtr(nullptr), mLen(0), mHasValue(hasValue), mIsNull(isNull) {
    }
    SplStringPiece(const char* ptr, int32_t len)
        : mPtr(ptr), mLen(len), mHasValue(true), mIsNull(false) {
    }

    SplStringPiece(const char* ptr)
        : SplStringPiece(ptr, strlen(ptr)) {
    }

    SplStringPiece(const SplStringPiece& p) {
        this->mPtr = p.mPtr;
        this->mLen = p.mLen;
        this->mHasValue = p.mHasValue;
        this->mIsNull = p.mIsNull;
    }

    SplStringPiece(const re2::StringPiece& value)
        : SplStringPiece(value.data(), value.size()) {
    }

    SplStringPiece(const std::string& value)
        : SplStringPiece(value.data(), value.size()) {
    }
    SplStringPiece(const std::string* value)
        : SplStringPiece(value->data(), value->size()) {
    }

    SplStringPiece(char* stringBuffers, const char* from, const int32_t len) {
        // should make sure, stringBuffers length
        for (auto i = 0; i < len; ++i) {
            stringBuffers[i] = from[i];
        }
        mPtr = stringBuffers;
        mLen = len;
        mHasValue = true;
        mIsNull = false;
    }

    virtual ~SplStringPiece() {
        mPtr = nullptr;
        mLen = 0;
    }

    bool operator==(const SplStringPiece& other) const {
        if (!mHasValue && !other.mHasValue) {
            return true;
        }
        if (mPtr == other.mPtr && mLen == other.mLen) {
            return true;
        }
        return memcmp(mPtr, other.mPtr, mLen) == 0;
    }

    void set(const char* ptr, int32_t len) {
        mPtr = ptr;
        mLen = len;
        mHasValue = true;
        mIsNull = false;
    }

    bool empty() const {
        return !mHasValue || mLen == 0;
    }

    bool hasValue() const {
        return mHasValue;
    }

    bool isNull() const {
        return mHasValue && mIsNull;
    }

    std::string ToString() const {
        if (mPtr == nullptr) {
            return std::string();
        }
        return std::string(mPtr, mLen);
    }
    size_t hash() const;
};

std::ostream& operator<<(std::ostream& os, SplStringPiece p);

}  // namespace apsara::sls::spl

namespace std {
template <>
struct hash<apsara::sls::spl::SplStringPiece> {
    size_t operator()(const apsara::sls::spl::SplStringPiece& p) const {
        return p.hash();
    }
};
}  // namespace std