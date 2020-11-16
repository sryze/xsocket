project(xsocket)

cmake_minimum_required(VERSION 2.8)

option(XSOCKET_BUILD_EXAMPLES "Build xsocket library examples" OFF)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_library(xsocket STATIC src/xsocket.c include/xsocket.h)

if(XSOCKET_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
