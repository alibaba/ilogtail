CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT (sls_spl)

file(GLOB LIB_SOURCE_FILES *.cpp *.h .c)
append_source_files(LIB_SOURCE_FILES)
add_library(${PROJECT_NAME} STATIC ${LIB_SOURCE_FILES})

# replace CMAKE_C_COMPILE/CMAKE_CXX_COMPILE with your own

#SET(CMAKE_C_COMPILER "gcc")
#SET(CMAKE_CXX_COMPILER "g++")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -Wno-sign-compare -mavx --warn-no-sign-compare -Wunused-variable -D_GLIBCXX_USE_CXX11_ABI=0 -D__GLIBCXX_BITSIZE_INT_N_0=128 -D__GLIBCXX_TYPE_INT_N_0=__int128 -DNDEBUG=1 -DSPL_LOG_CUSTOM=1")
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++") 
#SET(CMAKE_BUILD_TYPE "Release")

INCLUDE_DIRECTORIES(./include)
LINK_DIRECTORIES(./lib)

#add_executable(example_main ${SRC_LIST})

TARGET_LINK_LIBRARIES(
    pipeline
    task
    rw 
    util
    cmd
    logger
    plan
    vector
    thirdparty

    presto_adapters
    presto_common
    presto_exception
    presto_http
    presto_log_rotate
    presto_prometheus
    presto_server_lib
    presto_sls_lib
    presto_sls_rpc
    presto_sls_rpc_protocol
    presto_thrift-cpp2
    presto_thrift_extra
    presto_types
    velox_tpch_connector
    velox_hive_connector
    presto_protocol
    presto_type_converter
    presto_operators
    velox_exec

    velox_functions_prestosql
    velox_functions_prestosql_impl
    velox_aggregates
    velox_arrow_bridge
    velox_buffer
    velox_codegen
    velox_common_hyperloglog
    velox_connector
    velox_core
    velox_config
    velox_coverage_util
    velox_dwio_catalog_fbhive
    
    velox_dwio_dwrf_reader
    velox_dwio_dwrf_writer
    velox_dwio_dwrf_utils
    velox_dwio_dwrf_common
    velox_dwio_dwrf_proto
    velox_dwio_common
    velox_dwio_common_compression
    velox_dwio_common_encryption
    velox_dwio_common_exception
    velox_dwio_parquet_reader
    velox_dwio_parquet_writer
    velox_encode
    velox_exception
    velox_presto_serializer
    velox_caching
    velox_expression
    velox_expression_functions
    velox_external_date
    velox_file
    velox_parse_expression
    velox_function_registry
    velox_functions_aggregates
    velox_functions_json
    velox_functions_lib
    velox_functions_util
    velox_functions_window
    velox_hive_partition_function
    velox_is_null_functions
    velox_memory
    
    velox_parse_parser
    velox_parse_utils
    velox_process
    velox_row_fast
    velox_rpc
    velox_serialization
    velox_test_util
    velox_time
    velox_tpch_gen
    velox_type
    velox_type_calculation
    velox_type_fbhive
    velox_type_tz
    velox_vector
    velox_vector_fuzzer
    velox_window
    velox_common_base
    velox_duckdb_parser
    velox_duckdb_allocator
    velox_duckdb_conversion
    velox_duckdb_functions
    
    velox_presto_types
    dbgen
    duckdb
    http_filters
    md5
    sls_connector_proto
    sls_project_version
    tpch_extension
    
    fmt
    proxygen
    proxygenhttpserver
    glog
    gflags
    folly
    uuid
    slsprotobuf
    zstd
    gtest
    boost_context
    boost_thread
    boost_regex
    re2
    
    double-conversion
    sodium
    fizz
    wangle
    antlr4-runtime
    thriftcpp2
    thrift-core
    thriftprotocol
    thriftmetadata
    transport
    prometheus-cpp-core
    prometheus-cpp-pull
    simdjson
    snappy
    bz2
    lzo2
    y
    event
    lz4
    z
    curl
    ssl
    crypto
    dl
    -pthread
    )
