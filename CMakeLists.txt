cmake_minimum_required(VERSION 2.8.4)

project(RISC-V)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   find_package(fmt REQUIRED)
   set( FMTLIB "fmt::fmt" )
else()
   set( FMTLIB )
endif()

include_directories( src )

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.c)
file(GLOB_RECURSE HEADER_FILES src/*.h)

add_executable(RISC-V-Emulator ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(RISC-V-Emulator
    PRIVATE
    PUBLIC
   ${FMTLIB})

