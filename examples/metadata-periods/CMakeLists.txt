cmake_minimum_required(VERSION 3.16)
project(metadata-periods-example CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(
        ascnd-client
        GIT_REPOSITORY https://github.com/ascnd-gg/ascnd-client-cpp.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(ascnd-client)
endif()

add_executable(metadata-periods main.cpp)
target_link_libraries(metadata-periods PRIVATE ascnd-client)
