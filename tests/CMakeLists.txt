# Test configuration for Ascnd C++ SDK

include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Include Google Test's CMake functions
include(GoogleTest)

# Result type tests (header-only, no dependencies)
add_executable(result_test
    result_test.cpp
)
target_include_directories(result_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(result_test PRIVATE
    GTest::gtest_main
    protobuf::libprotobuf
)
target_compile_features(result_test PRIVATE cxx_std_17)

gtest_discover_tests(result_test)

# Config validation tests
add_executable(config_test
    config_test.cpp
)
target_include_directories(config_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(config_test PRIVATE
    GTest::gtest_main
    ascnd-client
)
target_compile_features(config_test PRIVATE cxx_std_17)

gtest_discover_tests(config_test)

# Client unit tests (requires mock - basic tests for now)
add_executable(client_test
    client_test.cpp
)
target_include_directories(client_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(client_test PRIVATE
    GTest::gtest_main
    ascnd-client
)
target_compile_features(client_test PRIVATE cxx_std_17)

gtest_discover_tests(client_test)

# Logging tests
add_executable(logging_test
    logging_test.cpp
)
target_include_directories(logging_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(logging_test PRIVATE
    GTest::gtest_main
    ascnd-client
)
target_compile_features(logging_test PRIVATE cxx_std_17)

gtest_discover_tests(logging_test)
