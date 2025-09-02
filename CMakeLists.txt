cmake_minimum_required(VERSION 3.10)

project(exercises)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(ARCH x64)

# 3rd party libraries
add_subdirectory(lib/SDL)
include_directories(lib/stb)

# repo's executables
add_subdirectory("Session1(Introduction)")
add_subdirectory("Session2(Rendering)")
