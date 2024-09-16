const fs = require('fs')
const path = require('path')
const { spawnSync } = require('child_process')
const prebuilds = require('../prebuilds')

fs.rmSync(path.join(__dirname, 'addons'), { recursive: true, force: true })

for (const pkg of prebuilds()) {
  const cwd = pkg.root

  const device = framework(pkg, path.join(pkg.prebuilds, 'ios'), [
    path.join(pkg.prebuilds, 'ios-arm64', `${pkg.name}.bare`)
  ], { cwd })

  const simulator = framework(pkg, path.join(pkg.prebuilds, 'ios-simulator'), [
    path.join(pkg.prebuilds, 'ios-arm64-simulator', `${pkg.name}.bare`),
    path.join(pkg.prebuilds, 'ios-x64-simulator', `${pkg.name}.bare`)
  ], { cwd })

  xcframework(pkg, path.join(__dirname, 'addons'), [
    device,
    simulator
  ], { cwd })
}

function framework (pkg, dest, inputs, opts = {}) {
  const {
    cwd
  } = opts

  const name = `${pkg.name}.${pkg.version}`

  const framework = path.join(dest, `${name}.framework`)

  fs.mkdirSync(framework, { recursive: true })

  spawnSync('lipo', [
    '-create',
    '-output', path.join(framework, name),
    ...inputs
  ], { cwd })

  spawnSync('install_name_tool', [
    '-id', `@rpath/${name}.framework/${name}`,
    path.join(framework, name)
  ], { cwd })

  fs.writeFileSync(path.join(framework, 'Info.plist'), plist(pkg.name, pkg.version))

  return framework
}

function xcframework (pkg, dest, inputs, opts = {}) {
  const name = `${pkg.name}.${pkg.version}`

  const xcframework = path.join(dest, `${name}.xcframework`)

  fs.rmSync(xcframework, { recursive: true, force: true })

  spawnSync('xcodebuild', [
    '-create-xcframework',
    '-output', xcframework,
    ...inputs.flatMap((i) => ['-framework', i])
  ])

  return xcframework
}

function plist (name, version) {
  return `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleIdentifier</key>
  <string>${name}.${version}</string>
  <key>CFBundleVersion</key>
  <string>${version}</string>
  <key>CFBundleShortVersionString</key>
  <string>${version}</string>
  <key>CFBundleExecutable</key>
  <string>${name}.${version}</string>
  <key>CFBundlePackageType</key>
  <string>FMWK</string>
  <key>CFBundleSignature</key>
  <string>????</string>
</dict>
</plist>
`
}
