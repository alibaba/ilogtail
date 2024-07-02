//
// Created by 韩呈杰 on 2023/11/7.
//
#include <gtest/gtest.h>
#include <limits>
#include "common/SequenceCreator.h"

using namespace common;

TEST(CommonSequenceCreatorTest, getSequence) {
    SequenceCreator sc;
    EXPECT_EQ(1, sc.getSequence());
    EXPECT_EQ(2, sc.getSequence());
    EXPECT_EQ(3, sc.getSequence());

    // 溢出正确处理
    sc.m_seq = std::numeric_limits<decltype(sc.m_seq)>::max();
    EXPECT_EQ(1, sc.getSequence());
}