cmake_minimum_required(VERSION 3.22)
project(provider)

file(GLOB LIB_SOURCE_FILES *.cpp *.h)


# 添加包含路径
set(SUB_DIRECTORIES_LIST    
    logger
    profile_sender
    config/config_provider
)

foreach(DIR_NAME IN LISTS SUB_DIRECTORIES_LIST)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${DIR_NAME})
    file(GLOB TEMP_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${DIR_NAME}/*.cpp")
    list(APPEND SOURCE_FILES_LIST ${TEMP_SOURCE_FILES})
endforeach()


add_library(provider STATIC ${LIB_SOURCE_FILES})

# file(GLOB LIB_SOURCE_FILES *.cpp *.h)
# add_library(${PROJECT_NAME} STATIC ${LIB_SOURCE_FILES})

# target_link_libraries(${PROJECT_NAME} attributes_provider)
# target_link_libraries(${PROJECT_NAME} config_provider)
# target_link_libraries(${PROJECT_NAME} common_config_provider)
# target_link_libraries(${PROJECT_NAME} profile_sender)
# link_boost(${PROJECT_NAME})

# if (ENABLE_ENTERPRISE)
#     target_link_libraries(${PROJECT_NAME} config_sdk)
# else()
#     target_link_libraries(${PROJECT_NAME} config_server_pb)
# endif ()