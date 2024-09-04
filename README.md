# react-native-bare-kit

Bare Kit for React Native.

```sh
npm i react-native-bare-kit
```

## Usage

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

## License

Apache-2.0
