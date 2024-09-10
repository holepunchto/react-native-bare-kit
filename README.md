# react-native-bare-kit

Bare Kit for React Native.

```sh
npm i react-native-bare-kit
```

Working example is available in [bare-expo](https://github.com/holepunchto/bare-expo)

## Setup (from scratch)

1. create a RN project & install packages

```sh
npx create-expo-app demo --template blank
cd demo

# install required packages
npm install -s react-native-bare-kit events@npm:bare-events
```
2. create a development build locally

```sh
npx expo run:ios
```

or 

```sh 
npx expo run:android
```

> [!IMPORTANT]
> You may experience problems running the app on an emulated device under QEMU due to https://github.com/holepunchto/libjs/issues/4. If you encounter crashes, try running the app on a real device instead.
> if you got gradle imcompatible error, edit `android/build.gradle` and set `minSdkVersion` to `31`

Then you are ready to add bare related code in `demo/App.js`

## Usage

can refer https://github.com/holepunchto/bare-expo/blob/main/app/index.tsx to add UI side and Bare side code

```js
import { Worklet } from 'react-native-bare-kit'

const worklet = new Worklet()

await worklet.start('/app.bundle', /* Source for `app.bundle` */)

const rpc = new worklet.RPC((req) => {
  if (req.command === 'ping') {
    console.log(req.data)

    req.reply('pong')
  }
})

const req = rpc.request('ping')
req.send('ping')

const data = await req.reply()
console.log(data.toString())
```

## Debug

On Android can check bare related logs with command:

```sh
$ adb logcat "*:S bare:*"
```

## License

Apache-2.0
