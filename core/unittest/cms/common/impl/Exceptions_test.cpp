//
// Created by 韩呈杰 on 2023/11/20.
//
#include <gtest/gtest.h>
#include "common/Exceptions.h"

TEST(CommonExceptionsTest, ExpectedFailedException) {
    auto throwException = [](){
        throw ExpectedFailedException("hello");
    };

    EXPECT_THROW(throwException(), ExpectedFailedException);
}

TEST(CommonExceptionsTest, ExpectedFailedException02) {
    try {
        throw ExpectedFailedException("hello");
    } catch (const std::exception &ex) {
        EXPECT_STREQ("hello", ex.what());
    }
}

TEST(CommonExceptionsTest, OverflowException) {
    auto throwException = [](){
        throw OverflowException();
    };

    EXPECT_THROW(throwException(), OverflowException);
}
