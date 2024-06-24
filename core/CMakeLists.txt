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
project(logtail)

include(CMakeDependentOption)

# variable LINUX is supported in cmake version 3.25
# for now, explicitly set LINUX to true if the platform is linux
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX TRUE)
endif ()

# Options.
option(BUILD_LOGTAIL "Build Logtail executable and tools" ON)
option(BUILD_LOGTAIL_SHARED_LIBRARY "Build Logtail shared library")
option(ENABLE_ENTERPRISE "enable enterprise feature")
cmake_dependent_option(ENABLE_COMPATIBLE_MODE "Build Logtail in compatible mode (for low version Linux)" OFF "LINUX" OFF)
cmake_dependent_option(ENABLE_STATIC_LINK_CRT "Build Logtail by linking CRT statically" OFF "LINUX" OFF)
option(WITHOUTGDB "Build Logtail without gdb")
option(WITHOUTSPL "Build Logtail and UT without SPL")
option(BUILD_LOGTAIL_UT "Build unit test for Logtail")

if (ENABLE_ENTERPRISE)
    message(STATUS "Enable Enterprise Feature.")
    add_definitions(-D__ENTERPRISE__)
    include(${CMAKE_CURRENT_SOURCE_DIR}/enterprise_options.cmake)
else ()
    include(${CMAKE_CURRENT_SOURCE_DIR}/options.cmake)
endif ()

if (WITHOUTSPL)
    add_definitions(-D__EXCLUDE_SPL__)
endif()

# Default C/CXX flags.
if (UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fpic -fPIC -D_LARGEFILE64_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -fpic -fPIC -D_LARGEFILE64_SOURCE")
    if (NOT WITHOUTGDB)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -ggdb")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb")
    endif ()
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O1 -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1 -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    string(REPLACE "-O3" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-O3" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
elseif (MSVC)
    add_definitions(-DNOMINMAX)
    add_definitions(-DTARGETLIBS=Psapi.lib)
    add_definitions(-DPSAPI_VERSION=1)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /MP /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /MP")
endif ()

# To be compatible with low version Linux.
if (ENABLE_COMPATIBLE_MODE)
    message(STATUS "Enable compatible mode.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c90")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--wrap=memcpy")
    add_definitions(-DENABLE_COMPATIBLE_MODE)
endif ()

if (ANDROID OR CMAKE_BUILD_TYPE MATCHES Debug)
    set(NO_TCMALLOC TRUE)
    add_definitions(-DLOGTAIL_NO_TC_MALLOC)
endif ()

# Dependencies.
include(${CMAKE_CURRENT_SOURCE_DIR}/utils.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/dependencies.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/links.cmake)

# Subdirectories (modules).
set(SUB_DIRECTORIES_LIST
        batch application app_config checkpoint common compression config config/provider config/watcher config_manager config_server_pb 
        container_manager controller event event_handler event_listener file_server flusher go_pipeline input log_pb logger 
        models monitor parser pipeline plugin plugin/creator plugin/instance plugin/interface polling processor processor/daemon processor/inner
        profile_sender queue reader sdk sender serializer sls_control fuse
        )
if (LINUX)
    if (ENABLE_ENTERPRISE)
        set(SUB_DIRECTORIES_LIST ${SUB_DIRECTORIES_LIST} shennong streamlog)
    endif()
elseif(MSVC)
    if (ENABLE_ENTERPRISE)
        set(SUB_DIRECTORIES_LIST ${SUB_DIRECTORIES_LIST} daemon)
    endif()
endif ()
if (ENABLE_ENTERPRISE)
    set(SUB_DIRECTORIES_LIST ${SUB_DIRECTORIES_LIST} config_sdk)
endif()

# Module includes & set files.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("/opt/logtail_spl/include")
if (LINUX)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/observer)
    if (NOT WITHOUTSPL)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/spl)
    endif()
endif()

foreach (DIR_NAME ${SUB_DIRECTORIES_LIST})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${DIR_NAME})
endforeach (DIR_NAME)
foreach (DIR_NAME ${SUB_DIRECTORIES_LIST})
    file(GLOB TEMP_SOURCE_FILES ${DIR_NAME}/*.c ${DIR_NAME}/*.cc ${DIR_NAME}/*.cpp)
    list(APPEND SUB_SOURCE_FILES ${TEMP_SOURCE_FILES})
endforeach (DIR_NAME)

# remove several files in common
list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/BoostRegexValidator.cpp ${CMAKE_CURRENT_SOURCE_DIR}/common/GetUUID.cpp)
# remove several files in go_pipeline
list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/go_pipeline/LogtailPluginAdapter.cpp ${CMAKE_CURRENT_SOURCE_DIR}/go_pipeline/LogtailPluginAdapter.h)
# xxhash in common
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    file(GLOB XX_HASH_SOURCE_FILES common/xxhash/*.c common/xxhash/*.h)
else ()
    file(GLOB XX_HASH_SOURCE_FILES common/xxhash/xxhash.c common/xxhash/xxhash.h)
endif ()
list(APPEND SUB_SOURCE_FILES ${XX_HASH_SOURCE_FILES})
# memory in common
list(APPEND SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/memory/SourceBuffer.h)

if(MSVC)
    # remove LinuxDaemonUtil in common
    if (ENABLE_ENTERPRISE)
        list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/LinuxDaemonUtil.h ${CMAKE_CURRENT_SOURCE_DIR}/common/LinuxDaemonUtil.cpp)
    endif()
    # remove linux event listener
    file(GLOB REMOVE_EVENT_LISTENER_SOURCES event_listener/*_Linux.cpp event_listener/*_Linux.h)
    list(REMOVE_ITEM SUB_SOURCE_FILES ${REMOVE_EVENT_LISTENER_SOURCES})
    # remove observer related files in input
    list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/input/InputObserverNetwork.cpp ${CMAKE_CURRENT_SOURCE_DIR}/input/InputObserverNetwork.h)
    if (ENABLE_ENTERPRISE)
        list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/input/InputStream.cpp ${CMAKE_CURRENT_SOURCE_DIR}/input/InputStream.h)
    endif ()
    # remove spl related files in processor
    list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/processor/ProcessorSPL.cpp ${CMAKE_CURRENT_SOURCE_DIR}/processor/ProcessorSPL.h)
    set(WINDOWS_SOURCE_FILES ${SUB_SOURCE_FILES})
elseif(UNIX)
    # remove windows event listener
    file(GLOB REMOVE_EVENT_LISTENER_SOURCES event_listener/*_Windows.cpp event_listener/*_Windows.h)
    list(REMOVE_ITEM SUB_SOURCE_FILES ${REMOVE_EVENT_LISTENER_SOURCES})
    if (LINUX)
        # needed by observer in common
        file(GLOB PICOHTTPPARSER_SOURCE_FILES common/protocol/picohttpparser/*.c common/protocol/picohttpparser/*.h)
        list(APPEND SUB_SOURCE_FILES ${PICOHTTPPARSER_SOURCE_FILES})
        # observer
        file(GLOB_RECURSE APPEND_OBSERVER_SOURCES observer/*)
        list(APPEND SUB_SOURCE_FILES ${APPEND_OBSERVER_SOURCES})
        if(WITHOUTSPL)
            # remove spl related files in processor
            list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/processor/ProcessorSPL.cpp ${CMAKE_CURRENT_SOURCE_DIR}/processor/ProcessorSPL.h)
        endif()
    else()
        # remove observer related files in input
        list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/input/InputObserverNetwork.cpp ${CMAKE_CURRENT_SOURCE_DIR}/input/InputObserverNetwork.h)
        # remove inputStream in input
        if (ENABLE_ENTERPRISE)
            list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/input/InputStream.cpp ${CMAKE_CURRENT_SOURCE_DIR}/input/InputStream.h)
        endif ()
        # remove spl related files in processor
        list(REMOVE_ITEM SUB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/processor/ProcessorSPL.cpp ${CMAKE_CURRENT_SOURCE_DIR}/processor/ProcessorSPL.h)
    endif()
    set(UNIX_SOURCE_FILES ${SUB_SOURCE_FILES})
endif()

# Generate SPL library.
if (LINUX)
    if (NOT WITHOUTSPL)
        add_subdirectory(spl)
    endif()
endif()

# Logtail executable or shared library.
if (BUILD_LOGTAIL)
    if (ENABLE_ENTERPRISE)
        if (UNIX)
            add_executable(${LOGTAIL_TARGET} enterprise_logtail.cpp ${UNIX_SOURCE_FILES})
        elseif (MSVC)
            add_executable(${LOGTAIL_TARGET} enterprise_logtail_windows.cpp ${WINDOWS_SOURCE_FILES})
        endif ()
    else ()
        if (UNIX)
            add_executable(${LOGTAIL_TARGET} logtail.cpp ${UNIX_SOURCE_FILES})
        elseif (MSVC)
            add_executable(${LOGTAIL_TARGET} logtail_windows.cpp ${WINDOWS_SOURCE_FILES})
        endif ()
    endif()
endif()

if (BUILD_LOGTAIL_SHARED_LIBRARY)
    if (NOT WITHOUTSPL)
        message(ERROR, "generating logtail shared library is not supported with SPL.")
    else()
        if (ENABLE_ENTERPRISE)
            if (UNIX)
                add_library(${LOGTAIL_TARGET} SHARED enterprise_logtail.cpp ${UNIX_SOURCE_FILES})
            elseif (MSVC)
                add_library(${LOGTAIL_TARGET} SHARED enterprise_logtail_windows.cpp ${WINDOWS_SOURCE_FILES})
            endif ()
        else ()
            if (UNIX)
                add_library(${LOGTAIL_TARGET} SHARED logtail.cpp ${UNIX_SOURCE_FILES})
            elseif (MSVC)
                add_library(${LOGTAIL_TARGET} SHARED logtail_windows.cpp ${WINDOWS_SOURCE_FILES})
            endif ()
        endif()
    endif()
endif ()

# Logtail UT.
if (BUILD_LOGTAIL_UT)
    message(STATUS "Build unittest.")
    include(CTest)
    enable_testing()
    add_subdirectory(unittest)
endif ()

# Generate independent libraries.
# add_subdirectory(helper)
add_subdirectory(go_pipeline)
add_subdirectory(common)

# Link libraries.
if(BUILD_LOGTAIL OR BUILD_LOGTAIL_SHARED_LIBRARY)
    link_all(${LOGTAIL_TARGET})
endif()