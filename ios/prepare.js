const fs = require('fs')
const path = require('path')
const { spawnSync } = require('child_process')

const modules = path.join(__dirname, '..', '..', '..', 'node_modules')

for (const base of fs.readdirSync(modules)) {
  const cwd = path.join(modules, base)

  let pkg
  try {
    pkg = require(path.join(cwd, 'package.json'))
  } catch {
    continue
  }

  if (typeof pkg !== 'object' || pkg === null || pkg.addon === undefined) {
    continue
  }

  const prebuilds = path.join(cwd, 'prebuilds')

  const device = framework(pkg, path.join(prebuilds, 'ios'), [
    path.join(prebuilds, 'ios-arm64', `${pkg.name}.bare`)
  ], { cwd })

  const simulator = framework(pkg, path.join(prebuilds, 'ios-simulator'), [
    path.join(prebuilds, 'ios-arm64-simulator', `${pkg.name}.bare`),
    path.join(prebuilds, 'ios-x64-simulator', `${pkg.name}.bare`)
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
  <key>CFBundleDevelopmentRegion</key>
  <string>English</string>
  <key>CFBundleExecutable</key>
  <string>${name}-${version}</string>
  <key>CFBundleIconFile</key>
  <string></string>
  <key>CFBundleIdentifier</key>
  <string>${name}-${version}</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundlePackageType</key>
  <string>FMWK</string>
  <key>CFBundleSignature</key>
  <string>????</string>
  <key>CFBundleVersion</key>
  <string>${version}</string>
  <key>CFBundleShortVersionString</key>
  <string>${version}</string>
  <key>CSResourcesFileMapped</key>
  <true/>
</dict>
</plist>
`
}
