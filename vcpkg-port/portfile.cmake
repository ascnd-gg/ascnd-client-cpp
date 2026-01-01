vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ascnd-gg/ascnd-client-cpp
    REF "v${VERSION}"
    SHA512 0
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DASCND_BUILD_EXAMPLES=OFF
        -DASCND_USE_SYSTEM_JSON=ON
        -DASCND_USE_SYSTEM_HTTPLIB=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/ascnd-client)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
