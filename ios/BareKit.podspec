require "json"

package = JSON.parse(File.read(File.join(__dir__, "../package.json")))

Pod::Spec.new do |s|
  s.name = "BareKit"
  s.version = package["version"]
  s.summary = package["description"]
  s.homepage = package["homepage"]
  s.license = package["license"]
  s.authors = package["author"]

  s.platforms = { :ios => min_ios_version_supported }

  s.source = { :git => package["repository"]["url"], :tag => "#{s.version}" }

  s.vendored_frameworks = "*.xcframework", "addons/*.xcframework"
end

