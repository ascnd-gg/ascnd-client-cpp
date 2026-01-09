cmake_minimum_required(VERSION 3.16)
project(ascnd-client
    VERSION 1.1.1
    DESCRIPTION "C++ client library for the Ascnd leaderboard API"
    HOMEPAGE_URL "https://github.com/ascnd-gg/ascnd-client-cpp"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ASCND_BUILD_EXAMPLES "Build example applications" ON)
option(ASCND_BUILD_TESTS "Build unit tests" ON)
option(ASCND_INSTALL "Enable install targets" OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(glog CONFIG REQUIRED)

# Get gRPC plugin location
get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)

# Proto file paths
set(PROTO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PROTO_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/ascnd/gen")

# Create output directories
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})
file(MAKE_DIRECTORY ${PROTO_HEADER_DIR})

# Proto file
set(PROTO_FILE "${PROTO_SOURCE_DIR}/ascnd.proto")

# Generated file paths
set(PROTO_GENERATED_SRCS
    "${PROTO_BINARY_DIR}/ascnd.pb.cc"
    "${PROTO_BINARY_DIR}/ascnd.grpc.pb.cc"
)
set(PROTO_GENERATED_HDRS
    "${PROTO_BINARY_DIR}/ascnd.pb.h"
    "${PROTO_BINARY_DIR}/ascnd.grpc.pb.h"
)

# Custom command to generate protobuf and gRPC files
add_custom_command(
    OUTPUT ${PROTO_GENERATED_SRCS} ${PROTO_GENERATED_HDRS}
    COMMAND protobuf::protoc
        --proto_path=${PROTO_SOURCE_DIR}
        --cpp_out=${PROTO_BINARY_DIR}
        --grpc_out=${PROTO_BINARY_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating protobuf and gRPC C++ files"
)

# Copy headers to include directory for installation/distribution
add_custom_command(
    OUTPUT "${PROTO_HEADER_DIR}/ascnd.pb.h" "${PROTO_HEADER_DIR}/ascnd.grpc.pb.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROTO_BINARY_DIR}/ascnd.pb.h" "${PROTO_HEADER_DIR}/ascnd.pb.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROTO_BINARY_DIR}/ascnd.grpc.pb.h" "${PROTO_HEADER_DIR}/ascnd.grpc.pb.h"
    DEPENDS ${PROTO_GENERATED_HDRS}
    COMMENT "Copying generated headers to include directory"
)

# Custom target for proto generation
add_custom_target(ascnd_proto_gen DEPENDS
    ${PROTO_GENERATED_SRCS}
    ${PROTO_GENERATED_HDRS}
    "${PROTO_HEADER_DIR}/ascnd.pb.h"
    "${PROTO_HEADER_DIR}/ascnd.grpc.pb.h"
)

# Define the library
add_library(ascnd-client STATIC
    src/client.cpp
    ${PROTO_GENERATED_SRCS}
)

add_dependencies(ascnd-client ascnd_proto_gen)

target_include_directories(ascnd-client PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROTO_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(ascnd-client PRIVATE _WIN32_WINNT=0x0601)
endif()

# Link dependencies
target_link_libraries(ascnd-client PUBLIC
    Threads::Threads
    protobuf::libprotobuf
    gRPC::grpc++
    glog::glog
)

# Build examples
if(ASCND_BUILD_EXAMPLES)
    add_executable(ascnd-example examples/basic_usage.cpp)
    target_link_libraries(ascnd-example PRIVATE ascnd-client)

    # Stand-alone example projects
    add_subdirectory(examples/submit-score)
    add_subdirectory(examples/leaderboard)
    add_subdirectory(examples/metadata-periods)
endif()

# Build tests
if(ASCND_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
if(ASCND_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS ascnd-client
        EXPORT ascnd-client-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install generated proto headers
    install(FILES
        "${PROTO_BINARY_DIR}/ascnd.pb.h"
        "${PROTO_BINARY_DIR}/ascnd.grpc.pb.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ascnd
    )

    install(EXPORT ascnd-client-targets
        FILE ascnd-client-targets.cmake
        NAMESPACE ascnd::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ascnd-client
    )

    # Package configuration
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ascnd-client-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ascnd-client-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ascnd-client-config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ascnd-client
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ascnd-client-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ascnd-client-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ascnd-client
    )
endif()
