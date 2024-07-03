#include <gtest/gtest.h>
#include "cloudMonitor/sliding_window.h"

using namespace std;
using namespace cloudMonitor;

TEST(Cms_SlidingWindowTest, Test)
{
    SlidingWindow<int> slidingWindow(3);

    EXPECT_EQ(size_t(0), slidingWindow.AccumulateCount());
    EXPECT_EQ(0, slidingWindow.Size());

    vector<int> values;
    EXPECT_EQ(0, slidingWindow.Max());
    EXPECT_EQ(0, slidingWindow.Min());
    EXPECT_EQ(0.0, slidingWindow.Avg());

    slidingWindow.Update(1);
    EXPECT_EQ(size_t(1), slidingWindow.AccumulateCount());
    EXPECT_EQ(1, slidingWindow.Size());
    slidingWindow.GetValues(values);
    EXPECT_EQ(1, slidingWindow.Last());
    EXPECT_EQ(1, slidingWindow.Max());
    EXPECT_EQ(1, slidingWindow.Min());
    EXPECT_EQ(1.0, slidingWindow.Avg());
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(size_t(1), values.size());

    slidingWindow.Update(2);
    EXPECT_EQ(size_t(2), slidingWindow.AccumulateCount());
    EXPECT_EQ(2, slidingWindow.Size());
    EXPECT_EQ(2, slidingWindow.Max());
    EXPECT_EQ(1, slidingWindow.Min());
    EXPECT_EQ(1.5, slidingWindow.Avg());
    slidingWindow.GetValues(values);
    EXPECT_EQ(size_t(2), values.size());
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);

    slidingWindow.Update(3);
    EXPECT_EQ(size_t(3), slidingWindow.AccumulateCount());
    EXPECT_EQ(3, slidingWindow.Size());
    EXPECT_EQ(3, slidingWindow.Max());
    EXPECT_EQ(1, slidingWindow.Min());
    EXPECT_EQ(2.0, slidingWindow.Avg());
    slidingWindow.GetValues(values);
    EXPECT_EQ(size_t(3), values.size());
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);

    slidingWindow.Update(4);
    EXPECT_EQ(size_t(4), slidingWindow.AccumulateCount());
    EXPECT_EQ(3, slidingWindow.Size());
    EXPECT_EQ(4, slidingWindow.Max());
    EXPECT_EQ(2, slidingWindow.Min());
    EXPECT_EQ(3.0, slidingWindow.Avg());
    slidingWindow.GetValues(values);
    EXPECT_EQ(values[0], 4);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);
}
