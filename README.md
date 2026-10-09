# react-native-bare-kit

<https://github.com/holepunchto/bare-kit> for React Native.

```
npm i react-native-bare-kit
```

## Usage

```js
import { Worklet } from 'react-native-bare-kit'
import b4a from 'b4a'

const worklet = new Worklet()

const source = `\
const { IPC } = BareKit

IPC.on('data', (data) => console.log(data.toString()))
IPC.write(Buffer.from('Hello from Bare!'))
`

worklet.start('/app.js', source)

const { IPC } = worklet

IPC.on('data', (data) => console.log(b4a.toString(data)))
IPC.write(b4a.from('Hello from React Native!'))
```

Alternatively to load from a bundle:

```js
import { Worklet } from 'react-native-bare-kit'

// Bundle output by `bare-pack`
// Extension can be .bundle, .js, .cjs, .mjs
import bundle from './my.bundle.js'

const worklet = new Worklet()
// First arg (filename)'s extension *must* be .bundle
worklet.start('/app.bundle', source)

// [...]
```

Refer to <https://github.com/holepunchto/bare-expo> for an example of using the library in an Expo application.

### Linking native addons

Native addons used by the worklet are not linked automatically. Applications must link them into their native projects themselves, such as by using <https://github.com/holepunchto/bare-link>. Given the entry point of the worklet, `bare-link` links the addons loaded by its module graph:

```console
bare-link --preset ios --out ios/addons worklet.js
bare-link --preset android --out android/app/src/main/jniLibs worklet.js
```

On iOS, the resulting `.xcframework` bundles must be embedded in the application target. On Android, the resulting `.so` libraries must be included in the `jniLibs` of the application and the resulting `.jar` libraries, if any, added as dependencies.

As a convenience, the library includes designated directories that are already wired into its native projects. Any `.xcframework` bundle in `ios/addons` and any `.so` or `.jar` library in `android/src/main/addons` of the library will be included in the build without further configuration:

```console
bare-link --preset ios --out node_modules/react-native-bare-kit/ios/addons worklet.js
bare-link --preset android --out node_modules/react-native-bare-kit/android/src/main/addons worklet.js
```

As these directories live within `node_modules`, they are cleared whenever the library is reinstalled. Consider running the commands from a `postinstall` script of the application:

```json
{
  "scripts": {
    "postinstall": "npm run link:ios && npm run link:android",
    "link:ios": "bare-link --preset ios --out node_modules/react-native-bare-kit/ios/addons worklet.js",
    "link:android": "bare-link --preset android --out node_modules/react-native-bare-kit/android/src/main/addons worklet.js"
  },
  "devDependencies": {
    "bare-link": "^4.0.1"
  }
}
```

### Logging

The `console.*` logging APIs used in the worklet write to the system log using <https://github.com/holepunchto/liblog> with the `bare` identifier. Refer to <https://github.com/holepunchto/liblog#consuming-logs> for instructions on how to consume the logs.

## License

Apache-2.0
