cmake_minimum_required(VERSION 3.5)

# if(WIN32)
# add_definitions(-D_WIN32)
# endif()

set(SOURCE_FILES
        # src/fmt.cc
        src/format.cc
        src/os.cc)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

ADD_LIBRARY(fmt STATIC ${SOURCE_FILES})
