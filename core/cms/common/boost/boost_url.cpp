// In exactly *one* source file
// this macro will be redefined in ur
#if defined(_MSC_VER) && defined(WIN32_LEAN_AND_MEAN)
#	undef WIN32_LEAN_AND_MEAN
#endif
#if !defined(DISABLE_BOOST_URL)
#include <boost/url/src.hpp>
#endif
