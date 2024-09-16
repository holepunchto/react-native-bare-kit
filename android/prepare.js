const fs = require('fs')
const path = require('path')
const prebuilds = require('../prebuilds')

const [abi] = process.argv.slice(2)

let arch

switch (abi) {
  case 'arm64-v8a': arch = 'arm64'; break
  case 'armeabi-v7a': arch = 'arm'; break
  case 'x86': arch = 'ia32'; break
  case 'x86_64': arch = 'x64'; break
  default: throw new Error(`unknown abi '${abi}'`)
}

const addons = path.join(__dirname, 'src', 'main', 'addons', abi)

fs.rmSync(addons, { recursive: true, force: true })
fs.mkdirSync(addons, { recursive: true })

for (const pkg of prebuilds()) {
  fs.copyFileSync(path.join(pkg.prebuilds, `android-${arch}`, `${pkg.name}.bare`), path.join(addons, `lib${pkg.name}.${pkg.version}.so`))
}
