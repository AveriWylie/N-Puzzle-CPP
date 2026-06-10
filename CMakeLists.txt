cmake_minimum_required(VERSION 3.16)
project(npuzzle CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# raylib: use an installed copy if present, otherwise fetch it
find_package(raylib 4.5 QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

# main visualizer
add_executable(npuzzle src/main.cpp src/visuals.cpp)
target_link_libraries(npuzzle PRIVATE raylib)

# headless correctness test (no raylib dependency)
add_executable(npuzzle_test test.cpp)
target_include_directories(npuzzle_test PRIVATE src)
