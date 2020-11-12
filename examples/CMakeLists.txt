include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(http_get http_get.c )

target_link_libraries(http_get xsocket)
if(WIN32)
  target_link_libraries(http_get ws2_32)
endif()
