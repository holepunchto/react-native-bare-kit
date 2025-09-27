cmake_minimum_required(VERSION 3.22)

project(react-native-bare-kit)

add_library(bare-kit SHARED IMPORTED)

set_target_properties(
  bare-kit
  PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/bare-kit/jni/${ANDROID_ABI}/libbare-kit.so"
)

add_library(react-native-bare-kit STATIC)

set_target_properties(
  react-native-bare-kit
  PROPERTIES
  CXX_STANDARD 20
  CXX_SCAN_FOR_MODULES OFF
)

target_sources(
  react-native-bare-kit
  PRIVATE
    ../shared/BareKitModule.cc
    ../shared/BareKitModule.h
)

target_include_directories(
  react-native-bare-kit
  PUBLIC
    ../shared
)

target_link_libraries(
  react-native-bare-kit
  bare-kit
  react_codegen_BareKitSpec
)
