# Copyright 2024 iLogtail Authors
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

macro(link_all target_name)
    link_jsoncpp(${target_name})
    link_yamlcpp(${target_name})
    link_boost(${target_name})
    link_gflags(${target_name})
    link_lz4(${target_name})
    link_zlib(${target_name})
    link_zstd(${target_name})
    link_unwind(${target_name})
    if (BUILD_LOGTAIL_UT)
        link_gtest(${target_name})
    endif ()
    link_re2(${target_name})
    link_protobuf(${target_name})
    link_cityhash(${target_name})
    link_leveldb(${target_name})
    link_asan(${target_name})
    if (LINUX AND NOT WITHOUTSPL)
        link_spl(${target_name})
    endif ()
    link_curl(${target_name})
    link_tcmalloc(${target_name})
    if (UNIX)
        link_uuid(${target_name})
        target_link_libraries(${target_name} dl)
        if (LINUX)
            if (NOT WITHOUTSPL)
                target_link_libraries(${target_name} spl)
            endif ()
            target_link_libraries(${target_name} pthread)
            if (ENABLE_ENTERPRISE)
                target_link_libraries(${target_name} shennong)
                target_link_libraries(${target_name} streamlog)
            endif()
        endif ()
        if (ENABLE_COMPATIBLE_MODE)
            target_link_libraries(${target_name} rt -static-libstdc++ -static-libgcc)
        endif ()
        if (ENABLE_STATIC_LINK_CRT)
            target_link_libraries(${target_name} -static-libstdc++ -static-libgcc)
        endif ()
    elseif (MSVC)
        target_link_libraries(${target_name} "ws2_32.lib")
        target_link_libraries(${target_name} "Rpcrt4.lib")
        target_link_libraries(${target_name} "Shlwapi.lib")
        target_link_libraries(${target_name} "Psapi.lib")
    endif ()
    link_ssl(${target_name}) # must after link_spl
    link_crypto(${target_name}) # must after link_spl
endmacro()