cmake_minimum_required(VERSION 3.2.2)
project(netmd)
set(CMAKE_BUILD_TYPE Release)
set (CMAKE_CXX_STANDARD 11)
#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "*.c")

#Generate the shared library from the sources
add_library(snetmd STATIC ${SOURCES} "CMDiscHeader.cpp")
add_library(netmd SHARED ${SOURCES} "CMDiscHeader.cpp")

target_link_libraries(netmd "gcrypt")
target_link_libraries(netmd "usb-1.0")
target_link_libraries(netmd "gpg-error")
IF (WIN32)
    target_link_libraries(netmd "ws2_32")
ENDIF()

target_link_libraries(snetmd "gcrypt")
target_link_libraries(snetmd "usb-1.0")
target_link_libraries(snetmd "gpg-error")
IF (WIN32)
    target_link_libraries(snetmd "ws2_32")
ENDIF()

