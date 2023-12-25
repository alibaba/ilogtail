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

# Set dependencies root, we assume that all dependencies are installed with following structure:
# - ${DEPS_ROOT}
#	- include: DEPS_INCLUDE_ROOT
#	- lib: DEPS_LIBRARY_ROOT
#	- bin: DEPS_BINARY_ROOT
# You can set your own ${DEPS_ROOT} when calling cmake, sub-directories can also be set with
# corresponding variables.
if (UNIX)
    set(DEFAULT_DEPS_ROOT "/opt/logtail/deps")
elseif (MSVC)
    set(DEFAULT_DEPS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../logtail_deps")
endif ()
logtail_define(DEPS_ROOT "Root directory for dependencies to install" ${DEFAULT_DEPS_ROOT})
logtail_define(DEPS_INCLUDE_ROOT "" "${DEPS_ROOT}/include")
logtail_define(DEPS_LIBRARY_ROOT "" "${DEPS_ROOT}/lib")
logtail_define(DEPS_BINARY_ROOT "" "${DEPS_ROOT}/bin")
include_directories("${DEPS_INCLUDE_ROOT}")
link_directories("${DEPS_LIBRARY_ROOT}")

# Each dependency has three related variables can be set:
# - {dep_name}_INCLUDE_DIR
# - {dep_name}_LIBRARY_DIR
# - {dep_name}_LINK_OPTION: static link is used by default, you can set this variable to
#	library name only to change this behaviour.
set(INCLUDE_DIR_SUFFIX "INCLUDE_DIR")
set(LIBRARY_DIR_SUFFIX "LIBRARY_DIR")
set(LINK_OPTION_SUFFIX "LINK_OPTION")
# Dependencies list.
set(DEP_NAME_LIST
        spdlog                  # header-only
        rapidjson               # header-only
        gtest
        protobuf
        re2
        cityhash
        gflags
        jsoncpp
        yamlcpp
        boost
        lz4
        zlib
        zstd
        curl
        unwind                  # google breakpad on Windows
        ssl                     # openssl
        crypto
        dmiuuid
        leveldb
        )

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND DEP_NAME_LIST "tcmalloc") # (gperftools)
endif()

if (MSVC)
    if (NOT DEFINED unwind_${INCLUDE_DIR_SUFFIX})
        set(unwind_${INCLUDE_DIR_SUFFIX} ${DEPS_INCLUDE_ROOT}/breakpad)
    endif ()
endif ()

# Set link options, add user-defined INCLUDE_DIR and LIBRARY_DIR.
foreach (DEP_NAME ${DEP_NAME_LIST})
    logtaiL_define(${DEP_NAME}_${LINK_OPTION_SUFFIX} "Link option for ${DEP_NAME}" "")

    if (${DEP_NAME}_${INCLUDE_DIR_SUFFIX})
        include_directories("${${DEP_NAME}_${INCLUDE_DIR_SUFFIX}}")
    endif ()

    if (${DEP_NAME}_${LIBRARY_DIR_SUFFIX})
        link_directories("${${DEP_NAME}_${LIBRARY_DIR_SUFFIX}}")
    else ()
        set(${DEP_NAME}_${LIBRARY_DIR_SUFFIX} "${DEPS_LIBRARY_ROOT}")
    endif ()
endforeach (DEP_NAME)

# spdlog, replace implementation.
if (spdlog_${INCLUDE_DIR_SUFFIX})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog/sinks/rotating_file_sink-inl.h
            DESTINATION "${spdlog_${INCLUDE_DIR_SUFFIX}}/spdlog/sinks")
else ()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog/sinks/rotating_file_sink-inl.h
            DESTINATION ${DEPS_INCLUDE_ROOT}/spdlog/sinks)
endif ()

# gtest
macro(link_gtest target_name)
    if (gtest_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${gtest_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${gtest_${LIBRARY_DIR_SUFFIX}}/libgtest.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "gtestd"
                optimized "gtest")
    endif ()
endmacro()

# protobuf
macro(link_protobuf target_name)
    if (protobuf_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${protobuf_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${protobuf_${LIBRARY_DIR_SUFFIX}}/libprotobuf.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "libprotobufd"
                optimized "libprotobuf")
    endif ()
endmacro()
logtail_define(protobuf_BIN "Absolute path to protoc" "${DEPS_BINARY_ROOT}/protoc")
set(PROTO_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/log_pb")
set(PROTO_FILES ${PROTO_FILE_PATH}/sls_logs.proto ${PROTO_FILE_PATH}/logtail_buffer_meta.proto ${PROTO_FILE_PATH}/metric.proto ${PROTO_FILE_PATH}/checkpoint.proto)
execute_process(COMMAND ${protobuf_BIN} --proto_path=${PROTO_FILE_PATH} --cpp_out=${PROTO_FILE_PATH} ${PROTO_FILES})

# re2
macro(link_re2 target_name)
    if (re2_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${re2_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${re2_${LIBRARY_DIR_SUFFIX}}/libre2.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "re2d"
                optimized "re2")
    endif ()
endmacro()

# tcmalloc (gperftools)
macro(link_tcmalloc target_name)
    if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
        if (tcmalloc_${LINK_OPTION_SUFFIX})
            target_link_libraries(${target_name} "${tcmalloc_${LINK_OPTION_SUFFIX}}")
        elseif (UNIX)
            target_link_libraries(${target_name} "${tcmalloc_${LIBRARY_DIR_SUFFIX}}/libtcmalloc.a")
        elseif (MSVC)
            add_definitions(-DPERFTOOLS_DLL_DECL=)
            target_link_libraries(${target_name}
                    debug "libtcmalloc_minimald"
                    optimized "libtcmalloc_minimal")
        endif ()
    endif ()
endmacro()

# cityhash
macro(link_cityhash target_name)
    if (cityhash_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${cityhash_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${cityhash_${LIBRARY_DIR_SUFFIX}}/libcityhash.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "cityhashd"
                optimized "cityhash")
    endif ()
endmacro()

# gflags
macro(link_gflags target_name)
    if (gflags_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${gflags_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${gflags_${LIBRARY_DIR_SUFFIX}}/libgflags.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "gflags_staticd"
                optimized "gflags_static")
    endif ()
endmacro()

# jsoncpp
macro(link_jsoncpp target_name)
    if (jsoncpp_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${jsoncpp_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${jsoncpp_${LIBRARY_DIR_SUFFIX}}/libjsoncpp.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "jsoncppd"
                optimized "jsoncpp")
    endif ()
endmacro()

# yamlcpp
macro(link_yamlcpp target_name)
    if (yamlcpp_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${yamlcpp_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${yamlcpp_${LIBRARY_DIR_SUFFIX}}/libyaml-cpp.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "yaml-cppd"
                optimized "yaml-cpp")
    endif ()
endmacro()

# boost
macro(link_boost target_name)
    if (boost_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${boost_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name}
                "${boost_${LIBRARY_DIR_SUFFIX}}/libboost_regex.a"
                "${boost_${LIBRARY_DIR_SUFFIX}}/libboost_thread.a"
                "${boost_${LIBRARY_DIR_SUFFIX}}/libboost_system.a"
                "${boost_${LIBRARY_DIR_SUFFIX}}/libboost_filesystem.a"
                "${boost_${LIBRARY_DIR_SUFFIX}}/libboost_chrono.a"
                )
    elseif (MSVC)
        if (NOT DEFINED Boost_FOUND)
            set(Boost_USE_STATIC_LIBS ON)
            set(Boost_USE_MULTITHREADED ON)
            set(Boost_USE_STATIC_RUNTIME ON)
            find_package(Boost 1.59.0 REQUIRED COMPONENTS regex thread system filesystem)
        endif ()
        if (Boost_FOUND)
            include_directories(${Boost_INCLUDE_DIRS})
            link_directories(${Boost_LIBRARY_DIRS})
        endif ()
        target_link_libraries(${target_name} ${Boost_LIBRARIES})
    endif ()
endmacro()

# lz4
macro(link_lz4 target_name)
    if (lz4_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${lz4_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${lz4_${LIBRARY_DIR_SUFFIX}}/liblz4.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "liblz4_staticd"
                optimized "liblz4_static")
    endif ()
endmacro()

# zlib
macro(link_zlib target_name)
    if (zlib_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${zlib_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${zlib_${LIBRARY_DIR_SUFFIX}}/libz.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "zlibstaticd"
                optimized "zlibstatic")
    endif ()
endmacro()


# zstd
macro(link_zstd target_name)
    if (zstd_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${zstd_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${zstd_${LIBRARY_DIR_SUFFIX}}/libzstd.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "zstdstaticd"
                optimized "zstdstatic")
    endif ()
endmacro()

# libcurl
macro(link_curl target_name)
    if (curl_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${curl_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${curl_${LIBRARY_DIR_SUFFIX}}/libcurl.a")
        target_link_libraries(${target_name} ssl crypto)
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "libcurl-d"
                optimized "libcurl")
        add_definitions(-DCURL_STATICLIB)
        target_link_libraries(${target_name}
                debug "libeay32"
                optimized "libeay32")
        target_link_libraries(${target_name}
                debug "ssleay32"
                optimized "ssleay32")
        target_link_libraries(${target_name} "Ws2_32.lib")
        target_link_libraries(${target_name} "Wldap32.lib")
    endif ()
endmacro()

# libunwind & google breakpad
macro(link_unwind target_name)
    if (unwind_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${unwind_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${unwind_${LIBRARY_DIR_SUFFIX}}/libunwind.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "breakpad_commond.lib"
                optimized "breakpad_common.lib")
        target_link_libraries(${target_name}
                debug "breakpad_crash_generation_clientd.lib"
                optimized "breakpad_crash_generation_client.lib")
        target_link_libraries(${target_name}
                debug "breakpad_exception_handlerd.lib"
                optimized "breakpad_exception_handler.lib")
    endif ()
endmacro()

# ssl
macro(link_ssl target_name)
    if (ssl_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${ssl_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${ssl_${LIBRARY_DIR_SUFFIX}}/libssl.a")
    elseif (MSVC)
        #target_link_libraries (${target_name}
        #   debug "libcurl-d"
        #   optimized "libcurl")
    endif ()
endmacro()

# crypto
macro(link_crypto target_name)
    if (ssl_${LINK_OPTION_SUFFIX})
        target_link_libraries(${target_name} "${crypto_${LINK_OPTION_SUFFIX}}")
    elseif (UNIX)
        target_link_libraries(${target_name} "${crypto_${LIBRARY_DIR_SUFFIX}}/libcrypto.a")
    elseif (MSVC)
        #target_link_libraries (${target_name}
        #   debug "libcurl-d"
        #   optimized "libcurl")
    endif ()
endmacro()

# leveldb
macro(link_leveldb target_name)
    if (UNIX)
        target_link_libraries(${target_name} "${leveldb_${LIBRARY_DIR_SUFFIX}}/libleveldb.a")
    elseif (MSVC)
        target_link_libraries(${target_name}
                debug "leveldbd.lib"
                optimized "leveldb.lib")
    endif ()
endmacro()

# asan for debug
macro(link_asan target_name)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        target_compile_options(${target_name} PUBLIC -fsanitize=address)
        target_link_options(${target_name} PUBLIC -fsanitize=address -static-libasan)
    endif()
endmacro()

macro(link_spl target_name)
    logtail_define(spl_${target_name} "" "")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libspl.a")

    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_adapters.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_common.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_exception.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_http.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_log_rotate.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_prometheus.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_server_lib.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_sls_lib.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_sls_rpc.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_sls_rpc_protocol.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_thrift-cpp2.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_thrift_extra.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_types.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_tpch_connector.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_hive_connector.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_protocol.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_type_converter.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libpresto_operators.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_exec.a")

    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_functions_prestosql.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_functions_prestosql_impl.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_aggregates.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_arrow_bridge.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_buffer.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_codegen.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_common_hyperloglog.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_connector.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_core.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_config.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_coverage_util.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_catalog_fbhive.a")

    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_dwrf_reader.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_dwrf_writer.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_dwrf_utils.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_dwrf_common.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_dwrf_proto.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_common.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_common_compression.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_common_encryption.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_common_exception.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_parquet_reader.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_dwio_parquet_writer.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_encode.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_exception.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_presto_serializer.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_caching.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_expression.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_expression_functions.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_external_date.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_file.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_parse_expression.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_function_registry.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_functions_aggregates.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_functions_json.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_functions_lib.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_functions_util.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_functions_window.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_hive_partition_function.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_is_null_functions.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_memory.a")

    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_parse_parser.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_parse_utils.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_process.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_row_fast.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_rpc.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_serialization.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_test_util.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_time.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_tpch_gen.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_type.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_type_calculation.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_type_fbhive.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_type_tz.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_vector.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_vector_fuzzer.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_window.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_common_base.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_duckdb_parser.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_duckdb_allocator.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_duckdb_conversion.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_duckdb_functions.a")

    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libvelox_presto_types.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libdbgen.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libduckdb.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libhttp_filters.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libmd5.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libsls_connector_proto.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libsls_project_version.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libtpch_extension.a")

    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libproxygen.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libproxygenhttpserver.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libglog.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libfolly.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libslsprotobuf.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libuuid.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libzstd.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libfmt.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libboost_context.a")

    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libdouble-conversion.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libsodium.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libfizz.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libwangle.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libantlr4-runtime.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libthriftcpp2.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libthrift-core.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libthriftprotocol.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libthriftmetadata.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libtransport.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libprometheus-cpp-core.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libprometheus-cpp-pull.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libsimdjson.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libsnappy.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libbz2.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/liblzo2.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/liby.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libevent.a")
    target_link_libraries(${target_name} "/opt/logtail_spl/lib/libevent_pthreads.a")

endmacro()

