cmake_minimum_required(VERSION 3.6.3)
project(GStreamerTask1)

set(CMAKE_CXX_STANDARD 20)

include(FindPkgConfig)

find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)
pkg_search_module(opencv REQUIRED IMPORTED_TARGET opencv4)

add_library(
    gstyolofilter SHARED
    src/gstyolofilter.h
    src/gstyolofilter.cpp
    src/YoloObjectDetector.cpp
    src/YoloObjectDetector.h
)

set_target_properties(gstyolofilter PROPERTIES PREFIX "")

target_link_libraries(
    gstyolofilter
    PkgConfig::gstreamer
    PkgConfig::gstreamer-sdp
    PkgConfig::gstreamer-app
    PkgConfig::gstreamer-video
    PkgConfig::opencv
)

add_custom_command(
    TARGET gstyolofilter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources/
    ${CMAKE_CURRENT_BINARY_DIR}/
)

