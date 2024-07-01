# option(ONE_AGENT "build for one agent" ON)
# option(ENABLE_CLOUD_MONITOR "enable cloud-monitor" ON)
# # 这两个选项与ENABLE_CLOUD_MONITOR互斥的
# option(DISABLE_ALIMONITOR "disable alimonitor-module" ON)
# option(DISABLE_TIANJI "disable tianji-module" ON)
# 单测相关
# option(ENABLE_COVERAGE "enable unit test" OFF)
set(ENABLE_COVERAGE ${BUILD_LOGTAIL_UT})

# if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     # gperftools仅支持linux
#     option(ENABLE_GPERF_TOOLS "enable gperftools" OFF)
# endif()
# if (ENABLE_GPERF_TOOLS)
#     add_definitions(-DENABLE_GPERF_TOOLS)
# endif()

add_definitions(-DONE_AGENT)
add_definitions(
    -DENABLE_CLOUD_MONITOR
    -DDISABLE_ALIMONITOR
    -DDISABLE_TIANJI
)

IF(ENABLE_COVERAGE)
    add_definitions(
            -DENABLE_COVERAGE
            -DVIRTUAL=virtual
            # metrichub中discard了以下指标: system.task、system.udp、memory.swap、system.cpuCore
            -DENABLE_CMS_SYS_TASK -DENABLE_UDP_SESSION -DENABLE_MEM_SWAP -DENABLE_CPU_CORE
    )

    message(STATUS "ENABLE_COVERAGE[cms] ON")
ELSE()
    # 根据ai的建议: -DVIRTUAL，这种写法VIRTUAL会被默认定义为1，因此需要加『等号』
    add_definitions(-DVIRTUAL=)
    message(STATUS "ENABLE_COVERAGE[cms] OFF")
ENDIF()

add_definitions(
    -DONE_AGENT 
    -DWITHOUT_MINI_DUMP
    -DDISABLE_BOOST_URL # 暂时禁用boost url
)
# boost
add_definitions(
        -DBOOST_CHRONO_HEADER_ONLY -DBOOST_CHRONO_VERSION=2
        -DBOOST_URL_NO_LIB
        -DBOOST_JSON_NO_LIB # not mentioned in boost doc
        -DBOOST_FILESYSTEM_NO_DEPRECATED # 不使用deprecated的特性
        -DBOOST_REGEX_STANDALONE # boost::regex不依赖boost的其它库
        # -DBOOST_STACKTRACE_LINK # https://www.boost.org/doc/libs/master/doc/html/stacktrace/configuration_and_build.html
)
add_definitions(-DHAVE_SOCKPATH=1)
