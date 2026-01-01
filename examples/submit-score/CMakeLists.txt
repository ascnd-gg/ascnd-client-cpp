cmake_minimum_required(VERSION 3.16)
project(submit-score-example CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# When built standalone, fetch ascnd-client
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(
        ascnd-client
        GIT_REPOSITORY https://github.com/ascnd-gg/ascnd-client-cpp.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(ascnd-client)
endif()

add_executable(submit-score main.cpp)
target_link_libraries(submit-score PRIVATE ascnd-client)
