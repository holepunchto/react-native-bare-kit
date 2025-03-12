const { AppState } = require('react-native')
const { Duplex } = require('streamx')
const b4a = require('b4a')
const { default: NativeBareKit } = require('./specs/NativeBareKit')

class BareKitIPC extends Duplex {
  constructor(worklet) {
    super()

    this._worklet = worklet

    this._pendingOpen = null
  }

  _open(cb) {
    if (this._worklet._id === -1) this._pendingOpen = cb
    else cb(null)
  }

  async _read(cb) {
    let err = null
    try {
      const data = await NativeBareKit.read(this._worklet._id)

      this.push(b4a.from(data, 'base64'))
    } catch (e) {
      err = e
    }

    cb(err)
  }

  async _write(data, cb) {
    let err = null
    try {
      if (typeof data === 'string') data = b4a.from(data)

      data = b4a.toString(data, 'base64')

      await NativeBareKit.write(this._worklet._id, data)
    } catch (e) {
      err = e
    }

    cb(err)
  }

  _continueOpen(err) {
    if (this._pendingOpen === null) {
      if (err) this.destroy(err)
    } else {
      const cb = this._pendingOpen
      this._pendingOpen = null
      cb(err)
    }
  }

  toJSON() {
    return {}
  }
}

exports.Worklet = class BareKitWorklet {
  static _worklets = new Map()

  constructor(opts = {}) {
    const { memoryLimit = 0, assets = null } = opts

    this._id = -1
    this._memoryLimit = memoryLimit
    this._assets = assets
    this._ipc = new BareKitIPC(this)
  }

  get IPC() {
    return this._ipc
  }

  start(filename, source, encoding, args = []) {
    if (Array.isArray(source)) {
      args = source
      source = null
    } else if (Array.isArray(encoding)) {
      args = encoding
      encoding = null
    }

    if (typeof source === 'string') {
      if (encoding !== 'base64') {
        source = b4a.toString(b4a.from(source, encoding), 'base64')
      }
    } else if (source) {
      source = b4a.toString(source, 'base64')
    }

    let err = null
    try {
      this._id = NativeBareKit.start(
        filename,
        source,
        args,
        this._memoryLimit,
        this._assets
      )

      BareKitWorklet._worklets.set(this._id, this)
    } catch (e) {
      err = e
    }

    this._ipc._continueOpen(err)

    if (err) throw err
  }

  suspend(linger = 0) {
    NativeBareKit.suspend(this._id, linger)
  }

  resume() {
    NativeBareKit.resume(this._id)
  }

  terminate() {
    try {
      NativeBareKit.terminate(this._id)
    } finally {
      this._id = -1
    }
  }

  toJSON() {
    return {}
  }

  static _onstatechange(state) {
    switch (state) {
      case 'active':
        return this._onstateactive()
      case 'background':
        return this._onstatebackground()
    }
  }

  static async _onstateactive() {
    for (const [, worklet] of this._worklets) worklet.resume()
  }

  static async _onstatebackground() {
    for (const [, worklet] of this._worklets) worklet.suspend()
  }
}

const Worklet = exports.Worklet

AppState.addEventListener('change', Worklet._onstatechange.bind(Worklet))
