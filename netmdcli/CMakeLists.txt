cmake_minimum_required(VERSION 3.10.3)
project(netmdcli)
set (CMAKE_C_STANDARD 11)
set (CMAKE_VERBOSE_BUILD 1)

if(MSVC OR MSYS OR MINGW)
    set(WINDOWS TRUE)
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/libnetmd
)

link_directories(
    ${CMAKE_BINARY_DIR}/libnetmd
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

IF (WINDOWS)
	SET(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
ELSEIF(APPLE)
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
ELSEIF(LINUX)
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
ENDIF()

add_executable(netmdcli netmdcli.c)
target_link_libraries(netmdcli snetmd)
