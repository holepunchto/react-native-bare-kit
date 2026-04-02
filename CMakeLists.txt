cmake_minimum_required(VERSION 4.0)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(react-native-bare-kit)

fetch_package("https://github.com/holepunchto/bare-kit/releases/download/v2.0.0/prebuilds.zip" SOURCE_DIR bare-kit)

install(
  DIRECTORY "${bare-kit}/android/bare-kit"
  DESTINATION android/libs
)

install(
  DIRECTORY "${bare-kit}/ios/BareKit.xcframework"
  DESTINATION ios
)
