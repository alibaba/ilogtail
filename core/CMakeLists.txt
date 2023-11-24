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

cmake_minimum_required(VERSION 2.9)
project(logtail)

option(ENABLE_ENTERPRISE "enable enterprise feature" OFF)

if (ENABLE_ENTERPRISE)
    message(STATUS "Enable Enterprise Feature.")
    add_definitions(-D__ENTERPRISE__)
    include(${CMAKE_CURRENT_SOURCE_DIR}/enterprise_main.cmake)
else ()
    include(${CMAKE_CURRENT_SOURCE_DIR}/main.cmake)
endif ()

