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

include(cms.cmake)

# 严格的编译选项
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${DEFAULT_DEPS_ROOT}\" -Wno-unused-function -Wall")
endif()

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
# target_link_libraries(${PROJECT_NAME} /opt/logtail/deps/lib/libapr-1.a)
# target_link_libraries(${PROJECT_NAME} pipeline)
# target_link_libraries(${PROJECT_NAME} sender)
