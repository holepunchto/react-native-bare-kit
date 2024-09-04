const { NativeEventEmitter, NativeModules } = require('react-native')
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

  constructor () {
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

  async start (filename, source) {
    try {
      this._id = await NativeModules.BareKit.start(filename, source)

      BareKitWorklet._worklets.set(this._id, this)

      this._ipc._continueOpen(null)
    } catch (err) {
      this._ipc._continueOpen(err)

      throw err
    }
  }

  toJSON () {
    return {}
  }

  static _onipcdata (event) {
    const worklet = this._worklets.get(event.worklet)

    if (worklet) worklet._ipc.push(b4a.from(event.data, 'base64'))
  }
}

const emitter = new NativeEventEmitter(NativeModules.BareKit)

emitter.addListener('BareKitIPCData', Worklet._onipcdata.bind(Worklet))
