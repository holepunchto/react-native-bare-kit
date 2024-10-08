const { NativeEventEmitter, NativeModules, AppState } = require('react-native')
const { Duplex } = require('bare-stream')
const RPC = require('bare-rpc')
const b4a = require('b4a')

const IPC = class BareKitIPC extends Duplex {
  constructor (worklet) {
    super()

    this._worklet = worklet

    this._pendingOpen = null
  }

  _open (cb) {
    if (this._worklet._id === -1) this._pendingOpen = cb
    else cb(null)
  }

  async _write (chunk, encoding, cb) {
    try {
      await NativeModules.BareKit.write(this._worklet._id, b4a.toString(chunk, 'base64'))

      cb(null)
    } catch (err) {
      cb(err)
    }
  }

  _continueOpen (err) {
    if (this._pendingOpen === null) return
    const cb = this._pendingOpen
    this._pendingOpen = null
    cb(err)
  }

  toJSON () {
    return {}
  }
}

const Worklet = exports.Worklet = class BareKitWorklet {
  static _worklets = new Map()

  constructor (opts = {}) {
    const {
      memoryLimit = 0,
      assets = null
    } = opts

    this._memoryLimit = memoryLimit
    this._assets = assets

    this._id = -1

    const ipc = this._ipc = new IPC(this)

    this._rpc = class extends RPC {
      constructor (onrequest) {
        super(ipc, onrequest)
      }
    }
  }

  get IPC () {
    return this._ipc
  }

  get RPC () {
    return this._rpc
  }

  async start (filename, source, encoding, args = []) {
    if (Array.isArray(source)) {
      args = source
      source = null
    } else if (Array.isArray(encoding)) {
      args = encoding
      encoding = null
    }

    if (typeof source === 'string') source = b4a.from(source, encoding)

    try {
      this._id = await NativeModules.BareKit.start(filename, b4a.toString(source, 'base64'), args, this._memoryLimit, this._assets)

      BareKitWorklet._worklets.set(this._id, this)

      this._ipc._continueOpen(null)
    } catch (err) {
      this._ipc._continueOpen(err)

      throw err
    }
  }

  async suspend (linger = 0) {
    await NativeModules.BareKit.suspend(this._id, linger)
  }

  async resume () {
    await NativeModules.BareKit.resume(this._id)
  }

  async terminate () {
    try {
      await NativeModules.BareKit.terminate(this._id)
    } finally {
      this._id = -1
    }
  }

  toJSON () {
    return {}
  }

  static _onipcdata (event) {
    const worklet = this._worklets.get(event.worklet)

    if (worklet) worklet._ipc.push(b4a.from(event.data, 'base64'))
  }

  static _onstatechange (state) {
    switch (state) {
      case 'active': return this._onstateactive()
      case 'background': return this._onstatebackground()
    }
  }

  static async _onstateactive () {
    for (const [, worklet] of this._worklets) await worklet.resume()
  }

  static async _onstatebackground () {
    for (const [, worklet] of this._worklets) await worklet.suspend()
  }
}

const emitter = new NativeEventEmitter(NativeModules.BareKit)

emitter.addListener('BareKitIPCData', Worklet._onipcdata.bind(Worklet))

AppState.addEventListener('change', Worklet._onstatechange.bind(Worklet))
