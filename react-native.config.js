module.exports = {
  dependency: {
    platforms: {
      ios: {},
      android: {
        cxxModuleCMakeListsModuleName: 'react-native-bare-kit',
        cxxModuleCMakeListsPath: 'CMakeLists.txt',
        cxxModuleHeaderName: 'BareKitModule'
      }
    }
  }
}
