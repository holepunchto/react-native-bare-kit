require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name = package["name"]
  s.version = package["version"]
  s.summary = package["description"]
  s.homepage = package["homepage"]
  s.license = package["license"]
  s.authors = package["author"]

  s.platforms = { :ios => min_ios_version_supported }

  s.source = { :git => package["repository"]["url"], :tag => "#{s.version}" }

  s.source_files = "ios/**/*.{h,mm}", "shared/**/*.{h,cc}"

  s.vendored_frameworks = "ios/*.xcframework", "ios/addons/*.xcframework"

  s.header_dir = "BareKit"

  s.module_name = "react_native_bare_kit"

  install_modules_dependencies(s)
end
