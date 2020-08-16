project(xsocket)

cmake_minimum_required(VERSION 2.8)

option(XSOCKET_BUILD_EXAMPLES "Build xsocket library examples" OFF)

add_library(xsocket STATIC xsocket.c xsocket.h)

if(XSOCKET_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
