const path = require('path')
const link = require('bare-link')

link(path.join(__dirname, '..', '..', '..'), {
  target: ['ios-arm64', 'ios-arm64-simulator', 'ios-x64-simulator'],
  out: path.join(__dirname, 'addons')
})
