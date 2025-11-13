cmake_minimum_required(VERSION 3.22.1)

project("opencv_edge_detector")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV Configuration
# Option 1: If OpenCV is installed via Android SDK Manager or as a module
# Uncomment and set the path to your OpenCV installation:
# set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/../../../opencv-sdk/sdk/native/jni")
# find_package(OpenCV REQUIRED)

# Option 2: Manual OpenCV setup
# Download OpenCV Android SDK from https://opencv.org/releases/
# Extract and set the path below:
set(OpenCV_DIR "/Users/suryaps/Desktop/Dump/opencv-edge-detector/opencv-sdk/sdk/native/jni")
# Or use an environment variable:
# set(OpenCV_DIR $ENV{OPENCV_ANDROID_SDK}/sdk/native/jni)

# For now, we'll try to find OpenCV, but you may need to adjust the path
find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    message(WARNING "OpenCV not found. Please set OpenCV_DIR to point to OpenCV Android SDK native/jni directory")
    message(WARNING "Example: set(OpenCV_DIR \"/path/to/opencv-sdk/sdk/native/jni\")")
endif()

# Include directories
if(OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Add source files
add_library(
    opencv_edge_detector
    SHARED
    native_renderer.cpp
)

# Link libraries
if(OpenCV_FOUND)
    target_link_libraries(
        opencv_edge_detector
        ${OpenCV_LIBS}
        android
        EGL
        GLESv2
        log
    )
else()
    # Link without OpenCV (will fail at runtime if OpenCV not available)
    target_link_libraries(
        opencv_edge_detector
        android
        EGL
        GLESv2
        log
    )
    message(WARNING "Building without OpenCV - app will not work correctly!")
endif()

# Set compile flags
target_compile_options(opencv_edge_detector PRIVATE
    -Wall
    -Wextra
    -fexceptions
    -frtti
)

