require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

begin
  lockfile = File.read(File.join(__dir__, "../../package-lock.json"))
rescue
  begin
    lockfile = File.read(File.join(__dir__, "../../yarn.lock"))
  rescue
    lockfile = nil
  end
end

Pod::Spec.new do |s|
  s.name = package["name"]
  s.version = package["version"]
  s.summary = package["description"]
  s.homepage = package["homepage"]
  s.license = package["license"]
  s.authors = package["author"]

  s.platforms = { :ios => min_ios_version_supported }

  s.source = { :git => package["repository"]["url"], :tag => "#{s.version}" }

  s.source_files = "ios/**/*.{h,m}"

  s.vendored_frameworks = "ios/*.xcframework", "ios/addons/*.xcframework"

  s.prepare_command = "node ios/prepare"

  install_modules_dependencies(s)

  if lockfile
    sum = Digest::SHA256.hexdigest lockfile

    s.version = "#{s.version}+#{sum[0, 6]}"
  end
end
