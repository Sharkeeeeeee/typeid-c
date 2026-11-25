cmake_minimum_required(VERSION 3.14)
project(typeid VERSION 0.1.0 LANGUAGES C)

# Header-only library target: link against typeid::typeid, then in exactly one
# source file `#define TYPEID_IMPLEMENTATION` before `#include "typeid.h"`.
add_library(typeid INTERFACE)
add_library(typeid::typeid ALIAS typeid)
target_include_directories(typeid INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)
if(WIN32)
  target_link_libraries(typeid INTERFACE bcrypt)
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  include(CTest)
  if(BUILD_TESTING)
    add_executable(test_typeid tests/test_typeid.c)
    target_link_libraries(test_typeid PRIVATE typeid)
    add_test(NAME test_typeid COMMAND test_typeid)
  endif()
  add_executable(typeid_cli examples/typeid_cli.c)
  target_link_libraries(typeid_cli PRIVATE typeid)
  set_target_properties(typeid_cli PROPERTIES OUTPUT_NAME typeid)
endif()

install(FILES typeid.h DESTINATION include)
install(TARGETS typeid EXPORT typeidTargets)
install(EXPORT typeidTargets NAMESPACE typeid:: DESTINATION lib/cmake/typeid
        FILE typeidConfig.cmake)
