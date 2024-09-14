const fs = require('fs')
const path = require('path')

module.exports = function * prebuilds (root = path.join(__dirname, '..', '..')) {
  const modules = path.join(root, 'node_modules')

  let entries
  try {
    entries = fs.readdirSync(modules)
  } catch {
    return
  }

  for (const entry of entries) {
    const cwd = path.join(modules, entry)

    let pkg
    try {
      pkg = require(path.join(cwd, 'package.json'))
    } catch {
      continue
    }

    if (typeof pkg !== 'object' || pkg === null || pkg.addon === undefined) {
      continue
    }

    yield {
      name: pkg.name,
      version: pkg.version,
      root: cwd,
      prebuilds: path.join(cwd, 'prebuilds')
    }

    yield * prebuilds(cwd)
  }
}
