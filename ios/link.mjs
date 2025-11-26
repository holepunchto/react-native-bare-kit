import path from 'path'
import { fileURLToPath } from 'url'
import link from 'bare-link'

const __filename = fileURLToPath(import.meta.url)

for await (const resource of link(path.join(__filename, '..', '..', '..', '..'), {
  target: ['ios-arm64', 'ios-arm64-simulator', 'ios-x64-simulator'],
  out: path.join(__filename, '..', 'addons')
})) {
  console.log('Wrote', resource)
}
