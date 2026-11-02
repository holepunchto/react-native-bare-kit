# react-native-bare-kit

<https://github.com/holepunchto/bare-kit> for React Native.

```
npm i react-native-bare-kit
```

Bare Kit isn't included. The application brings it along with the addons and assets of its worklets, so that other code, such as a notification service extension, can share them. See [Linking addons and assets](#linking-addons-and-assets).

## Usage

```js
import { Worklet } from 'react-native-bare-kit'
import b4a from 'b4a'

const worklet = new Worklet()

const source = `\
Bare.IPC.on('data', (data) => console.log(data.toString()))
Bare.IPC.write(Buffer.from('Hello from Bare!'))
`

worklet.start('/app.js', source)

const { IPC } = worklet

IPC.on('data', (data) => console.log(b4a.toString(data)))
IPC.write(b4a.from('Hello from React Native!'))
```

Refer to <https://github.com/holepunchto/bare-expo> for an example of using the library in an Expo application.

### Packing workers

Pack a worker and its dependencies into a package with <https://github.com/holepunchto/bare-stow> and <https://github.com/holepunchto/bare-stow-target-react-native>. The worker exports a `start()` function that receives the IPC stream:

```js
module.exports = function start(ipc) {
  ipc.on('data', (data) => ipc.write(data))
}
```

```console
bare-stow pack --target react-native --out ./worker worker.js
```

Add the package as a dependency of the application and start the worker in a worklet:

```js
import worker from 'worker'

const { ipc } = await worker.start()

ipc.on('data', (data) => console.log(b4a.toString(data)))
ipc.write(b4a.from('Hello from React Native!'))
```

The package leaves out the addons and assets of the worker, so link them as described below.

### Linking addons and assets

Link the addons and assets of all of the worklets of the application once with <https://github.com/holepunchto/bare-stow>. It writes a Swift package and a Gradle module, which also bring along Bare Kit:

```console
bare-stow link --target bare-kit --language swift --out ios/BareKitResources worklet.js
bare-stow link --target bare-kit --language kotlin --language-option package=com.example.addons --out android/bare-kit-resources worklet.js
```

Add the Swift package and its `bare-stow` folder to the application target, and include the Gradle module in the application. In an Expo application, list the workers in the `workers` option of <https://github.com/holepunchto/expo-bare-kit> instead.

### Logging

The `console.*` logging APIs used in the worklet write to the system log using <https://github.com/holepunchto/liblog> with the `bare` identifier. Refer to <https://github.com/holepunchto/liblog#consuming-logs> for instructions on how to consume the logs.

## License

Apache-2.0
