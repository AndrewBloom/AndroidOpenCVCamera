cmake_minimum_required(VERSION 3.22.1)
project(AndroidOpenCVCamera)

# Make sure we use flags for OpenGL in cpp
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")

# OpenCV library
find_package(OpenCV 4)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV > 4 not found.")
endif()

MESSAGE("OPENCV VERSION:")
MESSAGE(${OpenCV_VERSION})

set(OPENGL_LIB GLESv3)

find_package(Bloom3dEngineNdk REQUIRED CONFIG)

add_library(native-lib SHARED
        native-lib.cpp
        common.hpp
        camera_engine.cpp
        cam_utils.cpp
        )

# Link libraries
target_link_libraries( native-lib
        ${OpenCV_LIBS}
        Bloom3dEngineNdk::Bloom3dEngineNdk
        ${OPENGL_LIB}
        android
        camera2ndk
        log )