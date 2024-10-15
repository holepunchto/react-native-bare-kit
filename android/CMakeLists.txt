cmake_minimum_required(VERSION 3.25)

project(react_native_bare_kit C)

file(GLOB kit libs/${ANDROID_ABI}/libbare-kit.so)

add_library(bare_kit SHARED IMPORTED)

set_target_properties(
  bare_kit
  PROPERTIES
  IMPORTED_LOCATION "${kit}"
)

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

execute_process(
  COMMAND node prepare ${ANDROID_ABI}
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
)

file(GLOB addons src/main/addons/${ANDROID_ABI}/*.so)

foreach(addon ${addons})
  cmake_path(GET addon STEM LAST_ONLY target)

  add_library(${target} SHARED IMPORTED)

  set_target_properties(
    ${target}
    PROPERTIES
    IMPORTED_LOCATION "${addon}"
    IMPORTED_NO_SONAME ON
  )

  target_link_libraries(
    bare_kit_addons
    PUBLIC
      ${target}
  )
endforeach()

set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS
    ../../../package-lock.json
    ../../../yarn.lock
)

target_link_libraries(
  bare_kit_addons
  PUBLIC
    bare_kit
)
