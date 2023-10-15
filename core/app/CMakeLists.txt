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
project(app)

file(GLOB LIB_SOURCE_FILES *.cpp *.h)
append_source_files(LIB_SOURCE_FILES)
add_library(${PROJECT_NAME} STATIC ${LIB_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} app_config)
target_link_libraries(${PROJECT_NAME} common)
target_link_libraries(${PROJECT_NAME} config)
target_link_libraries(${PROJECT_NAME} config_provider)
target_link_libraries(${PROJECT_NAME} Go_pipeline)
target_link_libraries(${PROJECT_NAME} logger)
target_link_libraries(${PROJECT_NAME} monitor)
target_link_libraries(${PROJECT_NAME} plugin)
target_link_libraries(${PROJECT_NAME} pipeline)
target_link_libraries(${PROJECT_NAME} processor)

if (UNIX)
    target_link_libraries(${PROJECT_NAME} observer)
endif ()
target_link_libraries(${PROJECT_NAME} sender)
if (ENABLE_ENTERPRISE AND UNIX)
    target_link_libraries(${PROJECT_NAME} shennong)
    target_link_libraries(${PROJECT_NAME} streamlog)
endif ()
link_tcmalloc(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} config_manager)
target_link_libraries(${PROJECT_NAME} controller)
target_link_libraries(${PROJECT_NAME} event_handler)
target_link_libraries(${PROJECT_NAME} checkpoint)
target_link_libraries(${PROJECT_NAME} file)


