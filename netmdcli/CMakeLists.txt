cmake_minimum_required(VERSION 3.10.3)
project(netmdcli)

include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

ExternalProject_Add(JsonC
    GIT_REPOSITORY https://github.com/json-c/json-c.git
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
)

include_directories(
    ${EXTERNAL_INSTALL_LOCATION}/include
    ${CMAKE_SOURCE_DIR}/libnetmd
)

link_directories(
    ${EXTERNAL_INSTALL_LOCATION}/lib
    ${CMAKE_BINARY_DIR}/libnetmd
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

IF (NOT APPLE)
	SET(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
ELSE()
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
ENDIF()

add_executable(netmdcli netmdcli.c)
add_dependencies(netmdcli JsonC)
target_link_libraries(netmdcli "snetmd" json-c)
