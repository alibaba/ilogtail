#include <boost/chrono/detail/inlined/chrono.hpp>  // 直接编译到本地，静态库的形式不灵光

#if defined(_MSC_VER)
// msvc下避免LNK4221 警告
void boost_chrono_msvc_lnk4221_avoid() {
}
#endif
