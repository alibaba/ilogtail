# Copyright 2023 iLogtail Authors
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
project(cms_unittest)

option(ENABLE_CLOUD_MONITOR "enable cloud-monitor" ON)
# 这两个选项与ENABLE_CLOUD_MONITOR互斥的
option(DISABLE_ALIMONITOR "disable alimonitor-module" ON)
option(DISABLE_TIANJI "disable tianji-module" ON)
# 单测相关
# option(ENABLE_COVERAGE "enable unit test" ON)
set(ENABLE_COVERAGE ${BUILD_LOGTAIL_UT})
option(ENABLE_DEBUG "enable debug" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # gperftools仅支持linux
    option(ENABLE_GPERF_TOOLS "enable gperftools" OFF)
endif()
if (ENABLE_GPERF_TOOLS)
    add_definitions(-DENABLE_GPERF_TOOLS)
endif()

IF(ENABLE_CLOUD_MONITOR)
    add_definitions(-DENABLE_CLOUD_MONITOR)
    message(STATUS "ENABLE_CLOUD_MONITOR[ut] ON")
ELSE()
    add_definitions(-DDISABLE_CLOUD_MONITOR)
    message(STATUS "ENABLE_CLOUD_MONITOR[ut] OFF")
ENDIF()
IF(DISABLE_ALIMONITOR)
    add_definitions(-DDISABLE_ALIMONITOR)
    message(STATUS "DISABLE_ALIMONITOR[ut] ON")
ELSE()
    add_definitions(-DENABLE_ALIMONITOR)
    message(STATUS "DISABLE_ALIMONITOR[ut] OFF")
ENDIF()
IF(DISABLE_TIANJI)
    add_definitions(-DDISABLE_TIANJI)
    message(STATUS "DISABLE_TIANJI[ut] ON")
ELSE()
    add_definitions(-DENABLE_TIANJI)
    message(STATUS "DISABLE_TIANJI[ut] OFF")
ENDIF()
IF(ENABLE_COVERAGE)
    add_definitions(
            -DENABLE_COVERAGE
            -DVIRTUAL=virtual
            # metrichub中discard了以下指标: system.task、system.udp、memory.swap、system.cpuCore
            -DENABLE_CMS_SYS_TASK -DENABLE_UDP_SESSION -DENABLE_MEM_SWAP -DENABLE_CPU_CORE
    )

    message(STATUS "ENABLE_COVERAGE[ut] ON")
ELSE()
    # 根据ai的建议: -DVIRTUAL，这种写法VIRTUAL会被默认定义为1，因此需要加『等号』
    add_definitions(-DVIRTUAL=)
    message(STATUS "ENABLE_COVERAGE[ut] OFF")
ENDIF()

add_definitions(-DWITHOUT_MINI_DUMP)
add_definitions(-DONE_AGENT 
            -DDISABLE_BOOST_URL # 暂时禁用boost url
            -DDISABLE_SLS
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-sign-compare")


file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.c*")
# sic
set(WINDOWS_SPEC_FILES win32_system_information_collector_test.cpp wmi_test.cpp)
set(LINUX_SPEC_FILES linux_system_information_collector_test.cpp)
set(DARWIN_SPEC_FILES system_information_collector_darwin_test.cpp)
set(UNEXPECTED_OS_SPEC_FILES ${WINDOWS_SPEC_FILES} ${LINUX_SPEC_FILES} ${DARWIN_SPEC_FILES})
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(REMOVE_ITEM UNEXPECTED_OS_SPEC_FILES ${LINUX_SPEC_FILES})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(REMOVE_ITEM UNEXPECTED_OS_SPEC_FILES ${WINDOWS_SPEC_FILES})
else()
    list(REMOVE_ITEM UNEXPECTED_OS_SPEC_FILES ${DARWIN_SPEC_FILES})
endif()
foreach(OS_SPEC_FILE IN ITEMS ${UNEXPECTED_OS_SPEC_FILES})
    list(REMOVE_ITEM SOURCE_FILES ${PROJECT_SOURCE_DIR}/sic/tests/${OS_SPEC_FILE})
endforeach(OS_SPEC_FILE)

# message(STATUS "argus ut SOURCE FILES: ${SOURCE_FILES}")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
link_argus(${PROJECT_NAME})

set(CMS_SRC ${PROJECT_SOURCE_DIR}/../../cms)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMS_SRC}
    ${CMS_SRC}/third_party/fmt/include
    ${CMS_SRC}/third_party/header_only
    ${DEPS_INCLUDE_ROOT}/sigar-1.6
)

# boost_iostreams is depended by ExecCmd function
target_link_libraries(${PROJECT_NAME} sigar unittest_base boost_iostreams boost_program_options pthread)

# if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#     target_link_options(${PROJECT_NAME} PRIVATE -Wl,--wrap=__gxx_personality_v0 -Wl,--wrap=pthread_create -Wl,--wrap=__gxx_personality_v0 -rdynamic)
# endif()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
