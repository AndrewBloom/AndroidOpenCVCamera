cmake_minimum_required(VERSION 3.4.1)
project(AndroidOpenCVCamera)

# Make sure we use flags for OpenGL in cpp
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

#Added by IDE on project create
set(CPP_ROOT src/main/cpp)
set(SRC_FILES ${CPP_ROOT}/common.hpp ${CPP_ROOT}/native-lib.cpp)
add_library( native-lib SHARED ${SRC_FILES} )

# OpenCV library
find_package(OpenCV 4)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV > 4 not found.")
endif()

MESSAGE("OPENCV VERSION:")
MESSAGE(${OpenCV_VERSION})

# Link libraries
target_link_libraries( native-lib
        ${OpenCV_LIBS}
        android
        log )