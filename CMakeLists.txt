cmake_minimum_required(VERSION 3.16)
project(ascnd-client
    VERSION 1.0.0
    DESCRIPTION "C++ client library for the Ascnd leaderboard API"
    HOMEPAGE_URL "https://github.com/ascnd-gg/ascnd-client-cpp"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ASCND_BUILD_EXAMPLES "Build example applications" ON)
option(ASCND_HEADER_ONLY "Use header-only mode (no static library)" OFF)
option(ASCND_USE_SYSTEM_JSON "Use system-installed nlohmann_json" OFF)
option(ASCND_USE_SYSTEM_HTTPLIB "Use system-installed cpp-httplib" OFF)
option(ASCND_INSTALL "Enable install targets (requires system dependencies)" OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the library
if(ASCND_HEADER_ONLY)
    add_library(ascnd-client INTERFACE)
    target_include_directories(ascnd-client INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_definitions(ascnd-client INTERFACE ASCND_HEADER_ONLY)
else()
    add_library(ascnd-client STATIC
        src/client.cpp
    )
    target_include_directories(ascnd-client PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
endif()

# Platform-specific settings
if(WIN32)
    if(NOT ASCND_HEADER_ONLY)
        target_compile_definitions(ascnd-client PRIVATE _WIN32_WINNT=0x0601)
    else()
        target_compile_definitions(ascnd-client INTERFACE _WIN32_WINNT=0x0601)
    endif()
endif()

# Find or fetch nlohmann_json
if(ASCND_USE_SYSTEM_JSON)
    find_package(nlohmann_json 3.9.0 REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Find or fetch cpp-httplib
if(ASCND_USE_SYSTEM_HTTPLIB)
    find_package(httplib REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.15.3
        GIT_SHALLOW TRUE
    )
    set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(httplib)
endif()

# Link dependencies
if(ASCND_HEADER_ONLY)
    target_link_libraries(ascnd-client INTERFACE nlohmann_json::nlohmann_json httplib::httplib)
else()
    target_link_libraries(ascnd-client PUBLIC nlohmann_json::nlohmann_json httplib::httplib)
endif()

# OpenSSL for HTTPS support (optional but recommended)
find_package(OpenSSL QUIET)
if(OpenSSL::SSL)
    if(ASCND_HEADER_ONLY)
        target_link_libraries(ascnd-client INTERFACE OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(ascnd-client INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
    else()
        target_link_libraries(ascnd-client PUBLIC OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(ascnd-client PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
    endif()
    message(STATUS "OpenSSL found - HTTPS support enabled")
else()
    message(WARNING "OpenSSL not found - HTTPS support disabled. Install OpenSSL for production use.")
endif()

# Thread support
find_package(Threads REQUIRED)
if(ASCND_HEADER_ONLY)
    target_link_libraries(ascnd-client INTERFACE Threads::Threads)
else()
    target_link_libraries(ascnd-client PUBLIC Threads::Threads)
endif()

# Build examples
if(ASCND_BUILD_EXAMPLES)
    add_executable(ascnd-example examples/basic_usage.cpp)
    target_link_libraries(ascnd-example PRIVATE ascnd-client)
    
    # Stand-alone example projects
    add_subdirectory(examples/submit-score)
    add_subdirectory(examples/leaderboard)
    add_subdirectory(examples/metadata-periods)
endif()

# Installation (only when using system dependencies to avoid export issues)
if(ASCND_INSTALL)
    if(NOT ASCND_USE_SYSTEM_JSON OR NOT ASCND_USE_SYSTEM_HTTPLIB)
        message(WARNING "ASCND_INSTALL requires ASCND_USE_SYSTEM_JSON and ASCND_USE_SYSTEM_HTTPLIB to be ON. Skipping install targets.")
    else()
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
endif()
