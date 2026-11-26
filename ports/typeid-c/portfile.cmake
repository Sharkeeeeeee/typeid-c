# Overlay port for typeid-c.
#
# From a clone of this repository (builds the checked-out sources):
#   vcpkg install typeid-c --overlay-ports=path/to/typeid-c/ports
#
# Set TYPEID_C_REF to build a published tag instead, for example:
#   set(TYPEID_C_REF v0.1.0) and the matching TYPEID_C_SHA512.
if(DEFINED TYPEID_C_REF)
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO Sharkeeeeeee/typeid-c
        REF "${TYPEID_C_REF}"
        SHA512 "${TYPEID_C_SHA512}"
        HEAD_REF main
    )
else()
    get_filename_component(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DTYPEID_BUILD_TESTS=OFF
        -DTYPEID_BUILD_EXAMPLES=OFF
        -DTYPEID_INSTALL=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME typeid CONFIG_PATH lib/cmake/typeid)
vcpkg_fixup_pkgconfig()

# Header-only: nothing under debug/ or lib/.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug" "${CURRENT_PACKAGES_DIR}/lib")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE" "${SOURCE_PATH}/NOTICE")
