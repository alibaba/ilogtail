cmake_minimum_required(VERSION 3.22)
project(provider)

file(GLOB LIB_SOURCE_FILES *.cpp *.h)

set(PROVIDER_SUB_DIRECTORIES_LIST    
    profile_sender config/common_provider config_server_pb 
)

foreach(DIR_NAME IN LISTS PROVIDER_SUB_DIRECTORIES_LIST)
    file(GLOB TEMP_SOURCE_FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/../${DIR_NAME}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../${DIR_NAME}/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/../${DIR_NAME}/*.c"
        )
    list(APPEND LIB_SOURCE_FILES ${TEMP_SOURCE_FILES})
endforeach()


add_library(${PROJECT_NAME} STATIC ${LIB_SOURCE_FILES})

foreach(DIR_NAME IN LISTS PROVIDER_SUB_DIRECTORIES_LIST)
    target_include_directories(provider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../${DIR_NAME})
endforeach()

# message(STATUS "PROVIDER SOURCE FILES: ${LIB_SOURCE_FILES}")