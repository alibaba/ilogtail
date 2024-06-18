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
option(DISABLE_ALIMONITOR "disable alimonitor-module" OFF)
option(DISABLE_TIANJI "disable tianji-module" OFF)
# 单测相关
option(ENABLE_COVERAGE "enable unit test" OFF)
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
    message(STATUS "ENABLE_CLOUD_MONITOR ON")
ELSE()
    add_definitions(-DDISABLE_CLOUD_MONITOR)
    message(STATUS "ENABLE_CLOUD_MONITOR OFF")
ENDIF()
IF(DISABLE_ALIMONITOR)
    add_definitions(-DDISABLE_ALIMONITOR)
    message(STATUS "DISABLE_ALIMONITOR ON")
ELSE()
    add_definitions(-DENABLE_ALIMONITOR)
    message(STATUS "DISABLE_ALIMONITOR OFF")
ENDIF()
IF(DISABLE_TIANJI)
    add_definitions(-DDISABLE_TIANJI)
    message(STATUS "DISABLE_TIANJI ON")
ELSE()
    add_definitions(-DENABLE_TIANJI)
    message(STATUS "DISABLE_TIANJI OFF")
ENDIF()
IF(ENABLE_COVERAGE)
    add_definitions(
            -DENABLE_COVERAGE
            -DVIRTUAL=virtual
            # metrichub中discard了以下指标: system.task、system.udp、memory.swap、system.cpuCore
            -DENABLE_CMS_SYS_TASK -DENABLE_UDP_SESSION -DENABLE_MEM_SWAP -DENABLE_CPU_CORE
    )

    message(STATUS "ENABLE_COVERAGE ON")
ELSE()
    # 根据ai的建议: -DVIRTUAL，这种写法VIRTUAL会被默认定义为1，因此需要加『等号』
    add_definitions(-DVIRTUAL=)
    message(STATUS "ENABLE_COVERAGE OFF")
ENDIF()

add_executable(${PROJECT_NAME} CmsMain_test.cpp)

target_link_libraries(${PROJECT_NAME} unittest_base)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
