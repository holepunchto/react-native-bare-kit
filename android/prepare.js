const fs = require('fs')
const path = require('path')

const [abi] = process.argv.slice(2)

const modules = path.join(__dirname, '..', '..', '..', 'node_modules')

const addons = path.join(__dirname, 'src', 'main', 'addons', abi)

fs.rmSync(addons, { recursive: true, force: true })

fs.mkdirSync(addons)

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

  let arch

  switch (abi) {
    case 'arm64-v8a': arch = 'arm64'; break
    case 'armeabi-v7a': arch = 'arm'; break
    case 'x86': arch = 'ia32'; break
    case 'x86_64': arch = 'x64'; break
  }

  fs.copyFileSync(path.join(prebuilds, `android-${arch}`, `${pkg.name}.bare`), path.join(addons, `lib${pkg.name}.${pkg.version}.so`))
}
