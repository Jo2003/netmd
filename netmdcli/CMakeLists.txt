cmake_minimum_required(VERSION 3.20.2)
project(netmdcli)

#For the shared library:
link_directories( ${CMAKE_BINARY_DIR}/libnetmd )

SET(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}../winenv/lib/pkgconfig")

#Bring the headers, into the project
# include_directories(../libnetmd)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)

IF (NOT APPLE)
        SET(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}../winenv/lib/pkgconfig")
	SET(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
ELSE()
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
ENDIF()

pkg_check_modules(JSONC REQUIRED json-c)

set(IDIRS
    ${JSONC_INCLUDE_DIRS}
)

set(LDIRS
    ${JSONC_LIBRARY_DIRS}
)

set(SLIBS
    ${JSONC_LIBRARIES}
)

link_directories(${LDIRS})
include_directories(${IDIRS})

add_executable(netmdcli netmdcli.c)
target_link_libraries(netmdcli "snetmd" ${SLIBS})
