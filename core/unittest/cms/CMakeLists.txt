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

include(../../cms/cms.cmake)
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${DEFAULT_DEPS_ROOT}\" -Wno-unused-function -Wall")
endif()

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

# boost v1.79有点问题，BOOST_CATCH(x)定义成了catch(x)
set_source_files_properties("${PROJECT_SOURCE_DIR}/common/impl/TimeFormat_test.cpp" PROPERTIES COMPILE_FLAGS "-Wno-catch-value")

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
target_link_libraries(${PROJECT_NAME} unittest_base pthread)

if (MSVC)
    set(TEST_DATA_PATH ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/unittest_data)
else()
    set(TEST_DATA_PATH ${CMAKE_CURRENT_BINARY_DIR}/unittest_data)
endif()

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    # https://fig.io/manual/cmake/-E
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/unittest_data ${TEST_DATA_PATH}
)
# if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#     target_link_options(${PROJECT_NAME} PRIVATE -Wl,--wrap=__gxx_personality_v0 -Wl,--wrap=pthread_create -Wl,--wrap=__gxx_personality_v0 -rdynamic)
# endif()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
