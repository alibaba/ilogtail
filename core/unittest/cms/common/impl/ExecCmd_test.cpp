//
// Created by 韩呈杰 on 2024/5/8.
//
#include <gtest/gtest.h>
#include "common/ExecCmd.h"
#include "common/FilePathUtils.h"
#include <fmt/format.h>

TEST(CommonExecCmdTest, ExecCmd2String) {
#ifdef WIN32
#   define cmd R"(%SystemRoot%\System32\tasklist /nh)"
#else
#   define cmd "/bin/ls -l"
#endif
    {
        std::string out;
        EXPECT_EQ(0, ExecCmd(cmd, &out));
        std::cout << cmd << ":" << std::endl << out << std::endl;
        EXPECT_NE(out, "");
    }
#undef cmd
}

TEST(CommonExecCmdTest, ExecCmd2Vector) {
#ifdef WIN32
#   define cmd R"(%SystemRoot%\System32\tasklist /nh)"
#else
    std::string cmd = Which("ls").string() + " -l";
#endif
    {
        std::vector<std::string> out;
        EXPECT_EQ(0, ExecCmd(cmd, out));
        std::cout << cmd << ":" << std::endl;
        for (size_t i = 0; i < out.size(); i++) {
            std::cout << fmt::format("  [{:02d}] {}", i + 1, out[i]) << std::endl;
        }
        EXPECT_FALSE(out.empty());
    }
#ifdef cmd
#   undef cmd
#endif
}
