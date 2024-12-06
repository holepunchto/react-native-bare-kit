# react-native-bare-kit

<https://github.com/holepunchto/bare-kit> for React Native.

```
npm i react-native-bare-kit
```

## Usage

> [!IMPORTANT]
> You may experience problems using the library on an emulated Android device under QEMU due to https://github.com/holepunchto/libjs/issues/4. If you encounter crashes, try running your app on a real Android device instead.

```js
import { Worklet } from 'react-native-bare-kit'

const worklet = new Worklet()

const source = `\
const { IPC } = BareKit

IPC.setEncoding('utf8')
IPC.on('data', (data) => console.log(data))
IPC.write('Hello from Bare!')
`

await worklet.start('/app.js', source)

const { IPC } = worklet

IPC.setEncoding('utf8')
IPC.on('data', (data) => console.log(data))
IPC.write('Hello from React Native!')
```

Refer to <https://github.com/holepunchto/bare-expo> for an example of using the library in an Expo application.

### Logging

The `console.*` logging APIs used in the worklet write to the system log using <https://github.com/holepunchto/liblog> with the `bare` identifier. Refer to <https://github.com/holepunchto/liblog#consuming-logs> for instructions on how to consume the logs.

## License

Apache-2.0
