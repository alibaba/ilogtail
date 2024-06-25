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
project(cms)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-sign-compare")

file(GLOB_RECURSE LIB_SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.c*")
# sic
set(WINDOWS_SPEC_FILES system_information_collector_windows.cpp wmi.cpp win32_sic_utils.cpp)
set(LINUX_SPEC_FILES system_information_collector_linux.cpp)
set(DARWIN_SPEC_FILES system_information_collector_darwin.cpp)
set(UNEXPECTED_OS_SPEC_FILES ${WINDOWS_SPEC_FILES} ${LINUX_SPEC_FILES} ${DARWIN_SPEC_FILES})
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(REMOVE_ITEM UNEXPECTED_OS_SPEC_FILES ${LINUX_SPEC_FILES})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(REMOVE_ITEM UNEXPECTED_OS_SPEC_FILES ${WINDOWS_SPEC_FILES})
else()
    list(REMOVE_ITEM UNEXPECTED_OS_SPEC_FILES ${DARWIN_SPEC_FILES})
endif()
foreach(OS_SPEC_FILE IN ITEMS ${UNEXPECTED_OS_SPEC_FILES})
    list(REMOVE_ITEM LIB_SOURCE_FILES ${PROJECT_SOURCE_DIR}/sic/src/${OS_SPEC_FILE})
endforeach(OS_SPEC_FILE)
# message(STATUS "argus SOURCE FILE: ${LIB_SOURCE_FILES}")
list(REMOVE_ITEM LIB_SOURCE_FILES "${PROJECT_SOURCE_DIR}/third_party/fmt/src/fmt.cc")
set_source_files_properties("${PROJECT_SOURCE_DIR}/common/impl/Common.cpp" PROPERTIES COMPILE_DEFINITIONS "VERSION=\"4.0.0\"")
# message(STATUS "CMS_SOURCE_FILES: ${LIB_SOURCE_FILES}")
append_source_files(LIB_SOURCE_FILES) # append to unittest_base

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/fmt/include
    ${PROJECT_SOURCE_DIR}/third_party/header_only
    ${DEPS_INCLUDE_ROOT}/zlib
)
link_argus(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} boost_iostreams boost_program_options)
# target_link_libraries(${PROJECT_NAME} /opt/logtail/deps/lib/libapr-1.a)
# target_link_libraries(${PROJECT_NAME} pipeline)
# target_link_libraries(${PROJECT_NAME} sender)
