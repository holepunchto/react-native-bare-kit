const { AppState } = require('react-native')
const { Duplex } = require('streamx')
const { default: NativeBareKit } = require('./specs/NativeBareKit')

const constants = {
  STARTED: 0x1,
  TERMINATED: 0x2
}

class BareKitIPC extends Duplex {
  constructor(worklet) {
    super()

    this._worklet = worklet
    this._poll = this._poll.bind(this)

    this._pendingOpen = null
    this._pendingRead = null
    this._pendingWrite = null
  }

  _open(cb) {
    if (this._worklet._state & constants.STARTED) cb(null)
    else this._pendingOpen = cb
  }

  _update() {
    NativeBareKit.update(
      this._worklet._handle,
      this._pendingRead !== null,
      this._pendingWrite !== null
    )
  }

  _poll(readable, writable) {
    if (this._worklet._state & constants.TERMINATED) return
    if (readable) this._continueRead()
    if (writable) this._continueWrite()
  }

  _read(cb) {
    const data = NativeBareKit.read(this._worklet._handle)

    if (data) {
      this.push(new Uint8Array(data))
      cb(null)
    } else {
      this._pendingRead = cb
      this._update()
    }
  }

  _write(data, cb) {
    const written = NativeBareKit.write(
      this._worklet._handle,
      data.buffer,
      data.byteOffset,
      data.byteLength
    )

    if (written === data.byteLength) cb(null)
    else {
      this._pendingWrite = [data.subarray(written), cb]
      this._update()
    }
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

  _continueRead() {
    if (this._pendingRead === null) return
    const cb = this._pendingRead
    this._pendingRead = null
    this._update()
    this._read(cb)
  }

  _continueWrite() {
    if (this._pendingWrite === null) return
    const [data, cb] = this._pendingWrite
    this._pendingWrite = null
    this._update()
    this._write(data, cb)
  }

  toJSON() {
    return {}
  }
}

exports.Worklet = class BareKitWorklet {
  static _worklets = new Set()

  constructor(opts = {}) {
    const { memoryLimit = 0, assets = null } = opts

    if (typeof memoryLimit !== 'number') {
      throw new TypeError(
        'Memory limit must be a number. Received type ' +
          typeof memoryLimit +
          ' (' +
          memoryLimit +
          ')'
      )
    }

    if (typeof assets !== 'string' && assets !== null) {
      throw new TypeError(
        'Asset path must be a string. Received type ' +
          typeof assets +
          ' (' +
          assets +
          ')'
      )
    }

    this._state = 0
    this._source = null
    this._ipc = new BareKitIPC(this)

    this._handle = NativeBareKit.init(memoryLimit, assets, this._ipc._poll)
  }

  get IPC() {
    return this._ipc
  }

  start(filename, source, args = []) {
    if (typeof filename !== 'string') {
      throw new TypeError(
        'Filename must be a string. Received type ' +
          typeof filename +
          ' (' +
          filename +
          ')'
      )
    }

    if (Array.isArray(source)) {
      args = source
      source = null
    }

    if (
      source !== null &&
      typeof source !== 'string' &&
      !ArrayBuffer.isView(source)
    ) {
      throw new TypeError(
        'Source must be a string or TypedArray. Received type ' +
          typeof source +
          ' (' +
          source +
          ')'
      )
    }

    for (const arg of args) {
      if (typeof arg !== 'string') {
        throw new TypeError(
          'Argument must be a string. Received type ' +
            typeof arg +
            ' (' +
            arg +
            ')'
        )
      }
    }

    let err = null
    try {
      if (source === null) {
        NativeBareKit.startFile(this._handle, filename, args)
      } else if (typeof source === 'string') {
        NativeBareKit.startUTF8(this._handle, filename, source, args)
      } else {
        NativeBareKit.startBytes(
          this._handle,
          filename,
          source.buffer,
          source.byteOffset,
          source.byteLength,
          args
        )

        this._source = source // Keep a reference for lifetime management
      }

      this._state |= constants.STARTED

      BareKitWorklet._worklets.add(this)
    } catch (e) {
      err = e
    }

    this._ipc._continueOpen(err)

    if (err) throw err

    this.update()
  }

  suspend(linger = -1) {
    if (typeof linger !== 'number') {
      throw new TypeError(
        'Linger time must be a number. Received type ' +
          typeof linger +
          ' (' +
          linger +
          ')'
      )
    }

    NativeBareKit.suspend(this._handle, linger)
  }

  static suspend(linger) {
    for (const worklet of this._worklets) {
      worklet.suspend(linger)
    }
  }

  resume() {
    NativeBareKit.resume(this._handle)
  }

  static resume() {
    for (const worklet of this._worklets) {
      worklet.resume()
    }
  }

  update(state = AppState.currentState) {
    switch (state) {
      case 'active':
        return this.resume()
      case 'background':
        return this.suspend()
    }
  }

  static update(state) {
    for (const worklet of this._worklets) {
      worklet.update(state)
    }
  }

  get started() {
    return (this._state & constants.STARTED) !== 0
  }

  get terminated() {
    return (this._state & constants.TERMINATED) !== 0
  }

  terminate() {
    this._ipc.destroy()

    NativeBareKit.terminate(this._handle)

    this._state |= constants.TERMINATED
    this._source = null
    this._handle = null

    BareKitWorklet._worklets.delete(this)
  }

  toJSON() {
    return {}
  }
}

const Worklet = exports.Worklet

AppState.addEventListener('change', Worklet.update.bind(Worklet))
