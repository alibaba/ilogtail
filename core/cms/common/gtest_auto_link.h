#ifndef GTEST_AUTO_LINK_H
#define GTEST_AUTO_LINK_H

#if defined(_MSC_VER) || defined(MSVC)
#   if defined(_DEBUG) || !defined(NDEBUG)
#       pragma comment(lib, "gtestd.lib")
#       pragma comment(lib, "gtest_maind.lib")
#   else
#       pragma comment(lib, "gtest.lib")
#       pragma comment(lib, "gtest_main.lib")
#   endif
#endif

#endif // !GTEST_AUTO_LINK_H
