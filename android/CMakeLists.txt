cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(bare)

project(react_native_bare_kit C)

add_library(bare_kit_addons SHARED)

set_target_properties(
  bare_kit_addons
  PROPERTIES
  OUTPUT_NAME bare-kit-addons
)

target_sources(
  bare_kit_addons
  PUBLIC
    src/main/c/addons.c
)

link_bare_modules(
  bare_kit_addons
  PREBUILDS
  DESTINATION "${CMAKE_CURRENT_LIST_DIR}/src/main/jniLibs/${ANDROID_ABI}"
  WORKING_DIRECTORY ../../..
)

find_package(bare-kit REQUIRED CONFIG)

target_link_libraries(
  bare_kit_addons
  PUBLIC
    bare-kit::bare_kit
)
