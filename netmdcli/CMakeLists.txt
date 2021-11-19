cmake_minimum_required(VERSION 3.2.2)
project(netmdcli)

#For the shared library:
link_directories( libnetmd )

#Bring the headers, into the project
# include_directories(../libnetmd)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


SET(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

add_executable(netmdcli netmdcli.c)
target_link_libraries(netmdcli "snetmd")
target_link_libraries(netmdcli "json-c")
