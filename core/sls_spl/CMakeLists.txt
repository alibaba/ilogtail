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
project (sls_spl)

file(GLOB LIB_SOURCE_FILES *.cpp *.h .c)
append_source_files(LIB_SOURCE_FILES)
add_library(${PROJECT_NAME} STATIC ${LIB_SOURCE_FILES})


#set(SPL_DEPS_INCLUDE_ROOT "/opt/logtail_spl")
include_directories("/opt/logtail_spl/include")
link_directories("/opt/logtail_spl/lib")

set(DEP_NAME_LIST
        cmd                  # header-only
        curl               # header-only
        logger
        pipeline
        plan
        re2                # (gperftools)
        rw
        vector
        )

foreach (DEP_NAME ${DEP_NAME_LIST})
    logtaiL_define(SPL_${DEP_NAME} "" "/opt/logtail_spl/include/${DEP_NAME}")
    include_directories("/opt/logtail_spl/include/${DEP_NAME}")
endforeach (DEP_NAME)

link_slsspl(${PROJECT_NAME})