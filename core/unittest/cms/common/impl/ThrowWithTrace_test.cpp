#include <gtest/gtest.h>
#include "common/ThrowWithTrace.h"
#include "common/BoostStacktraceMt.h"

#include <exception>

TEST(CommonExceptionTest, ThrowWithTrace) {
	bool caught = false;
	try {
		ThrowWithTrace<std::logic_error>("by design");
	}
	catch (const std::exception &ex) {
		caught = true;
		const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(ex);
        ASSERT_NE(st, nullptr);
        std::cout << "ex.what(): " << ex;
	}
	EXPECT_TRUE(caught);
}

TEST(CommonExceptionTest, ThrowWithTrace2) {
    bool caught = false;
    try {
        ThrowWithTrace<std::logic_error>(std::string{"by design"});
    }
    catch (const std::exception &ex) {
        caught = true;
        const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(ex);
        ASSERT_NE(st, nullptr);
        std::cout << "ex.what(): " << ex;
    }
    EXPECT_TRUE(caught);
}

TEST(CommonExceptionTest, Throw) {
    bool caught = false;
    try {
        Throw<std::logic_error>("<{}>", "by design");
    }
    catch (const std::exception &ex) {
        caught = true;
        std::cout << "ex.what(): " << ex;
        EXPECT_STREQ(ex.what(), "<by design>");
    }
    EXPECT_TRUE(caught);
}
