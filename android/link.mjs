import path from 'path'
import { fileURLToPath } from 'url'
import link from 'bare-link'

const __filename = fileURLToPath(import.meta.url)

for await (const resource of link(path.join(__filename, '..', '..', '..', '..'), {
  target: ['android-arm64', 'android-arm', 'android-ia32', 'android-x64'],
  needs: ['libbare-kit.so'],
  out: path.join(__filename, '..', 'src', 'main', 'addons')
})) {
  console.log('Wrote', resource)
}
