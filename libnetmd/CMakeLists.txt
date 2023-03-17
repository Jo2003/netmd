cmake_minimum_required(VERSION 3.2.2)
project(netmd)
set(CMAKE_BUILD_TYPE Release)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_C_STANDARD 11)
set (CMAKE_VERBOSE_BUILD 1)

if(MSVC OR MSYS OR MINGW)
    set(WINDOWS TRUE)
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "*.c")

#Generate the shared library from the sources
add_library(snetmd STATIC ${SOURCES} "CMDiscHeader.cpp")
add_library(netmd SHARED ${SOURCES} "CMDiscHeader.cpp")

if(WTF)
    include(ExternalProject)
    set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

    ExternalProject_Add(libusb
        GIT_REPOSITORY https://github.com/libusb/libusb.git
        CONFIGURE_COMMAND bash ../libusb/bootstrap.sh && bash ../libusb/configure --prefix=${EXTERNAL_INSTALL_LOCATION} --enable-static --enable-shared
        BUILD_COMMAND make
        INSTALL_COMMAND make install
    )

    ExternalProject_Add(libgpgerr
        GIT_REPOSITORY git://git.gnupg.org/libgpg-error.git
        CONFIGURE_COMMAND cd ../libgpgerr && bash autoreconf -i --force && cd ../libgpgerr-build && bash ../libgpgerr/configure --prefix=${EXTERNAL_INSTALL_LOCATION} --enable-static --enable-shared
        BUILD_COMMAND make
        INSTALL_COMMAND make install
    )

    ExternalProject_Add(libgcrypt
        GIT_REPOSITORY git://git.gnupg.org/libgcrypt.git
        CONFIGURE_COMMAND cd ../libgcrypt && bash autoreconf -i --force && cd ../libgcrypt-build && bash ../libgcrypt/configure --prefix=${EXTERNAL_INSTALL_LOCATION} --enable-static --enable-shared
        BUILD_COMMAND make
        INSTALL_COMMAND make install
    )

    add_dependencies(libgcrypt libgpgerr)

    include_directories(
        ${EXTERNAL_INSTALL_LOCATION}/include
    )

    link_directories(
        ${EXTERNAL_INSTALL_LOCATION}/lib
    )

    add_dependencies(netmd libusb libgpgerr libgcrypt)

endif()

target_link_libraries(netmd gcrypt usb-1.0 gpg-error)
if(WINDOWS)
    target_link_libraries(netmd ws2_32)
ENDIF()

target_link_libraries(snetmd gcrypt usb-1.0 gpg-error)
if(WINDOWS)
    target_link_libraries(snetmd ws2_32)
ENDIF()
