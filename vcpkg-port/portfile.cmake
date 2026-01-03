vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ascnd-gg/ascnd-client-cpp
    REF "v${VERSION}"
    SHA512 7eb9dbd5804d687a6825ffd81f3a570a3e0c8e079f2fc56c7a117ada3f534b1ff6445eb4785f61855c3e8163fcc1aa89643164274017e28527933556ad0bd434
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DASCND_BUILD_EXAMPLES=OFF
        -DASCND_BUILD_TESTS=OFF
        -DASCND_INSTALL=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/ascnd-client)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
