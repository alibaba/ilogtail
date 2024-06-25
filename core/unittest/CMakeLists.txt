# Copyright 2022 iLogtail Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.22)
project(unittest_base)


# ----argusagent for tmp by @chengjie.hcj ---------------------------------------------------------------------------------
option(ONE_AGENT "build for one agent" ON)
option(ENABLE_CLOUD_MONITOR "enable cloud-monitor" ON)
# 这两个选项与ENABLE_CLOUD_MONITOR互斥的
option(DISABLE_ALIMONITOR "disable alimonitor-module" ON)
option(DISABLE_TIANJI "disable tianji-module" ON)
# 单测相关
# option(ENABLE_COVERAGE "enable unit test" OFF)
set(ENABLE_COVERAGE ${BUILD_LOGTAIL_UT})
option(ENABLE_DEBUG "enable debug" OFF)

# if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     # gperftools仅支持linux
#     option(ENABLE_GPERF_TOOLS "enable gperftools" OFF)
# endif()
# if (ENABLE_GPERF_TOOLS)
#     add_definitions(-DENABLE_GPERF_TOOLS)
# endif()

IF(ENABLE_CLOUD_MONITOR)
    add_definitions(-DENABLE_CLOUD_MONITOR)
    message(STATUS "ENABLE_CLOUD_MONITOR[cms] ON")
ELSE()
    add_definitions(-DDISABLE_CLOUD_MONITOR)
    message(STATUS "ENABLE_CLOUD_MONITOR[cms] OFF")
ENDIF()
IF(DISABLE_ALIMONITOR)
    add_definitions(-DDISABLE_ALIMONITOR)
    message(STATUS "DISABLE_ALIMONITOR[cms] ON")
ELSE()
    add_definitions(-DENABLE_ALIMONITOR)
    message(STATUS "DISABLE_ALIMONITOR[cms] OFF")
ENDIF()
IF(DISABLE_TIANJI)
    add_definitions(-DDISABLE_TIANJI)
    message(STATUS "DISABLE_TIANJI[cms] ON")
ELSE()
    add_definitions(-DENABLE_TIANJI)
    message(STATUS "DISABLE_TIANJI[cms] OFF")
ENDIF()
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

add_definitions(-DWITHOUT_MINI_DUMP)
add_definitions(-DONE_AGENT 
            -DDISABLE_BOOST_URL # 暂时禁用boost url
            -DDISABLE_SLS
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
set(CMS_SRC ${PROJECT_SOURCE_DIR}/../cms)
include_directories(
    ${CMS_SRC}
    ${CMS_SRC}/third_party/fmt/include
    ${CMS_SRC}/third_party/header_only
    ${DEPS_INCLUDE_ROOT}/zlib
    ${DEPS_INCLUDE_ROOT}/sigar-1.6
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-sign-compare")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--wrap=__gxx_personality_v0 -Wl,--wrap=pthread_create -Wl,--wrap=__gxx_personality_v0 -rdynamic")
# ----argusagent for tmp by @chengjie.hcj ---------------------------------------------------------------------------------

add_definitions(-DAPSARA_UNIT_TEST_MAIN)
set(NO_TCMALLOC TRUE)
add_definitions(-DLOGTAIL_NO_TC_MALLOC)
if (MSVC)
    add_definitions(-DNOMINMAX)
endif ()
if (LINUX AND WITHOUTSPL)
    add_library(${PROJECT_NAME} SHARED ${ALL_SOURCE_FILES})
else ()
    add_library(${PROJECT_NAME} STATIC ${ALL_SOURCE_FILES})
endif()
if(MSVC)
    # For MSVC, use /W4 for a high level of warnings, and treat warnings as errors.
    # target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
endif()
link_argus(${PROJECT_NAME})  # must before boost
link_jsoncpp(${PROJECT_NAME})
link_yamlcpp(${PROJECT_NAME})
link_boost(${PROJECT_NAME})
link_gflags(${PROJECT_NAME})
link_lz4(${PROJECT_NAME})
link_zlib(${PROJECT_NAME})
link_zstd(${PROJECT_NAME})
link_unwind(${PROJECT_NAME})
link_gtest(${PROJECT_NAME})
link_re2(${PROJECT_NAME})
link_protobuf(${PROJECT_NAME})
link_cityhash(${PROJECT_NAME})
link_leveldb(${PROJECT_NAME})
# link_asan(${PROJECT_NAME})
if (LINUX AND NOT WITHOUTSPL)
    link_spl(${PROJECT_NAME})
endif ()
link_curl(${PROJECT_NAME})
link_ssl(${PROJECT_NAME}) # must after link_spl
link_crypto(${PROJECT_NAME}) # must after link_spl
link_tcmalloc(${PROJECT_NAME})

if (UNIX)
    link_uuid(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} dl)
    if (LINUX)
        target_link_libraries(${PROJECT_NAME} pthread)
    endif ()
    if (ENABLE_COMPATIBLE_MODE)
        target_link_libraries(${PROJECT_NAME} rt -static-libstdc++ -static-libgcc)
    endif ()
elseif (MSVC)
    target_link_libraries(${PROJECT_NAME} "ws2_32.lib")
    target_link_libraries(${PROJECT_NAME} "Rpcrt4.lib")
    target_link_libraries(${PROJECT_NAME} "Shlwapi.lib")
endif ()

add_subdirectory(app_config)
add_subdirectory(batch)
add_subdirectory(checkpoint)
add_subdirectory(common)
add_subdirectory(compression)
add_subdirectory(config)
add_subdirectory(config_sdk)
add_subdirectory(container_manager)
add_subdirectory(controller)
add_subdirectory(event)
add_subdirectory(event_handler)
add_subdirectory(file_source)
add_subdirectory(flusher)
add_subdirectory(input)
add_subdirectory(log_pb)
add_subdirectory(models)
add_subdirectory(monitor)
add_subdirectory(pipeline)
add_subdirectory(plugin)
add_subdirectory(polling)
add_subdirectory(processor)
add_subdirectory(queue)
add_subdirectory(reader)
add_subdirectory(sdk)
add_subdirectory(sender)
add_subdirectory(cms)
add_subdirectory(serializer)

if (LINUX)
    if (NOT WITHOUTSPL)
        add_subdirectory(spl)
    endif ()
    add_subdirectory(observer)
endif ()