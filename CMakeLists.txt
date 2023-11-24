cmake_minimum_required(VERSION 2.8.4)

project(RISC-V)

#find_package(SDL2 REQUIRED)

#include_directories(${SDL2_INCLUDE_DIRS})
include_directories( src )

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.c)
file(GLOB_RECURSE HEADER_FILES src/*.h)

#set( SOURCE_FILES ${SOURCE_FILES} Emulator.cpp )

add_executable(RISC-V-Emulator ${HEADER_FILES} ${SOURCE_FILES})
#target_link_libraries(Masterlator ${SDL2_LIBRARIES})
