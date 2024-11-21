const path = require('path')
const link = require('bare-link')

link(path.join(__dirname, '..', '..', '..'), {
  target: ['android-arm64', 'android-arm', 'android-ia32', 'android-x64'],
  needs: ['libbare-kit.so'],
  out: path.join(__dirname, 'src', 'main', 'addons')
})
