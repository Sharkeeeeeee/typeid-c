cmake_minimum_required(VERSION 3.14)
project(typeid
  VERSION 0.1.0
  DESCRIPTION "TypeID for C: type-safe, K-sortable, globally unique IDs"
  HOMEPAGE_URL "https://github.com/Sharkeeeeeee/typeid-c"
  LANGUAGES C)

set(TYPEID_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(TYPEID_IS_TOP_LEVEL ON)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(TYPEID_BUILD_TESTS "Build the test suite" ${TYPEID_IS_TOP_LEVEL})
option(TYPEID_BUILD_EXAMPLES "Build the command-line example" ${TYPEID_IS_TOP_LEVEL})
option(TYPEID_INSTALL "Generate install rules" ${TYPEID_IS_TOP_LEVEL})

# Header-only library target: link against typeid::typeid, then in exactly one
# source file `#define TYPEID_IMPLEMENTATION` before `#include "typeid.h"`.
add_library(typeid INTERFACE)
add_library(typeid::typeid ALIAS typeid)
target_include_directories(typeid INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(typeid INTERFACE c_std_99)
if(WIN32)
  target_link_libraries(typeid INTERFACE bcrypt)
endif()

if(TYPEID_BUILD_TESTS)
  enable_testing()
  add_executable(test_typeid tests/test_typeid.c)
  target_link_libraries(test_typeid PRIVATE typeid)
  add_test(NAME test_typeid COMMAND test_typeid)

  include(CheckLanguage)
  check_language(CXX)
  if(CMAKE_CXX_COMPILER)
    enable_language(CXX)
    find_package(Threads REQUIRED)
    add_executable(test_typeid_cpp tests/test_typeid.cpp)
    target_link_libraries(test_typeid_cpp PRIVATE typeid Threads::Threads)
    target_compile_features(test_typeid_cpp PRIVATE cxx_std_17)
    add_test(NAME test_typeid_cpp COMMAND test_typeid_cpp)
  endif()
endif()

if(TYPEID_BUILD_EXAMPLES)
  add_executable(typeid_cli examples/typeid_cli.c)
  target_link_libraries(typeid_cli PRIVATE typeid)
  set_target_properties(typeid_cli PROPERTIES OUTPUT_NAME typeid)
endif()

if(TYPEID_INSTALL)
  install(FILES typeid.h typeid.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(TARGETS typeid EXPORT typeidTargets)
  install(EXPORT typeidTargets
    NAMESPACE typeid::
    FILE typeidTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/typeid)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/typeidConfig.cmake.in
    "@PACKAGE_INIT@\ninclude(\"\${CMAKE_CURRENT_LIST_DIR}/typeidTargets.cmake\")\n")
  configure_package_config_file(
    ${CMAKE_CURRENT_BINARY_DIR}/typeidConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/typeidConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/typeid)
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/typeidConfigVersion.cmake
    COMPATIBILITY SameMinorVersion ARCH_INDEPENDENT)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/typeidConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/typeidConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/typeid)
  configure_file(typeid.pc.in ${CMAKE_CURRENT_BINARY_DIR}/typeid.pc @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/typeid.pc
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig)
endif()
