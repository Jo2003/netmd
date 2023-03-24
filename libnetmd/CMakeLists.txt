cmake_minimum_required(VERSION 3.9)
project(netmd VERSION 1.0.1 DESCRIPTION "NetMD library")
set(CMAKE_BUILD_TYPE Release)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_VERBOSE_BUILD 1)

if(MSVC OR MSYS OR MINGW)
    set(WINDOWS TRUE)
endif()

set(LIB_SRC
    CMDiscHeader.cpp
    common.c
    error.c
    libnetmd_intern.c
    log.c
    netmd_dev.c
    netmd_transfer.c
    patch.c
    playercontrol.c
    secure.c
    trackinformation.c
    utils.c
)

add_library(netmd SHARED ${LIB_SRC})
target_compile_options(netmd PRIVATE -fPIC)

target_link_libraries(netmd usb-1.0 gcrypt gpg-error)

if (WINDOWS)
    target_link_libraries(netmd ws2_32)
endif()

set_target_properties(netmd PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/libnetmd.h)

target_include_directories(netmd PRIVATE .)

include(GNUInstallDirs)

install(TARGETS netmd
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(libnetmd.pc.in libnetmd.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libnetmd/libnetmd.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
