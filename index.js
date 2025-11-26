const { AppState } = require('react-native')
const { Duplex } = require('streamx')
const EventEmitter = require('bare-events')
const { default: NativeBareKit } = require('./specs/NativeBareKit')

const constants = {
  STARTED: 0x1,
  TERMINATED: 0x2,
  SUSPENDED: 0x4
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

  get worklet() {
    return this._worklet
  }

  toJSON() {
    return {
      worklet: this.worklet
    }
  }

  _open(cb) {
    if (this._worklet.started) cb(null)
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
    if (this._worklet.terminated) return
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
}

class BareKitWorklet extends EventEmitter {
  static _worklets = new Set()

  constructor(id = null, opts = {}) {
    if (typeof id === 'object' && id !== null) {
      opts = id
      id = null
    }

    if (typeof id !== 'string' && id !== null) {
      throw new TypeError('ID must be a string. Received type ' + typeof id + ' (' + id + ')')
    }

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
        'Asset path must be a string. Received type ' + typeof assets + ' (' + assets + ')'
      )
    }

    super()

    this._state = 0
    this._ipc = new BareKitIPC(this)
    this._inactiveTimeout = null

    const terminate = this.terminate.bind(this)

    this._handle = NativeBareKit.init(id, memoryLimit, assets, terminate, this._ipc._poll)
  }

  get IPC() {
    return this._ipc
  }

  get started() {
    return (this._state & constants.STARTED) !== 0
  }

  get terminated() {
    return (this._state & constants.TERMINATED) !== 0
  }

  get suspended() {
    return (this._state & constants.SUSPENDED) !== 0
  }

  start(filename, source, args = []) {
    if (this.started) throw new Error('Worklet has already been started')
    if (this.terminated) throw new Error('Worklet has been terminated')

    if (typeof filename !== 'string') {
      throw new TypeError(
        'Filename must be a string. Received type ' + typeof filename + ' (' + filename + ')'
      )
    }

    if (Array.isArray(source)) {
      args = source
      source = null
    }

    if (source !== null && typeof source !== 'string' && !ArrayBuffer.isView(source)) {
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
          'Argument must be a string. Received type ' + typeof arg + ' (' + arg + ')'
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
      }

      this._state |= constants.STARTED

      this.emit('start')

      BareKitWorklet._worklets.add(this)
    } catch (e) {
      err = e
    }

    this._ipc._continueOpen(err)

    if (err) throw err

    this.update()
  }

  suspend(linger = -1) {
    if (!this.started) throw new Error('Worklet has not been started')
    if (this.terminated) throw new Error('Worklet has been terminated')

    if (typeof linger !== 'number') {
      throw new TypeError(
        'Linger time must be a number. Received type ' + typeof linger + ' (' + linger + ')'
      )
    }

    NativeBareKit.suspend(this._handle, linger)

    this._state |= constants.SUSPENDED

    this.emit('suspend')
  }

  static suspend(linger) {
    for (const worklet of this._worklets) {
      worklet.suspend(linger)
    }
  }

  resume() {
    if (!this.started) throw new Error('Worklet has not been started')
    if (this.terminated) throw new Error('Worklet has been terminated')

    NativeBareKit.resume(this._handle)

    this._state &= ~constants.SUSPENDED

    this.emit('resume')
  }

  wakeup(deadline = 0) {
    if (!this.started) throw new Error('Worklet has not been started')
    if (this.terminated) throw new Error('Worklet has been terminated')

    if (typeof deadline !== 'number') {
      throw new TypeError(
        'Deadline time must be a number. Received type ' + typeof deadline + ' (' + deadline + ')'
      )
    }

    NativeBareKit.wakeup(this._handle, deadline)

    this.emit('wakeup')
  }

  static wakeup(deadline) {
    for (const worklet of this._worklets) {
      worklet.deadline(deadline)
    }
  }

  static resume() {
    for (const worklet of this._worklets) {
      worklet.resume()
    }
  }

  update(state = AppState.currentState) {
    if (this._inactiveTimeout) {
      clearTimeout(this._inactiveTimeout)

      this._inactiveTimeout = null
    }

    switch (state) {
      case 'active':
        return this.resume()
      case 'background':
        return this.suspend()
      case 'inactive':
        // We have some bug where we miss the suspension signal if we dont react
        // on the inactive state. The inactive state also fires on a bunch of
        // other stuff though, so we just "wiggle" the event here to buy time.
        this.suspend()

        this._inactiveTimeout = setTimeout(() => {
          this._inactiveTimeout = null

          if (AppState.currentState === 'inactive') this.resume()
        }, 500)
    }
  }

  static update(state) {
    for (const worklet of this._worklets) {
      worklet.update(state)
    }
  }

  terminate() {
    if (this.terminated) return

    this._ipc.destroy()

    if (this.started) NativeBareKit.terminate(this._handle)

    this._state |= constants.TERMINATED
    this._handle = null

    BareKitWorklet._worklets.delete(this)

    this.emit('terminate')
  }

  toJSON() {
    return {
      started: this.started,
      terminated: this.terminated,
      suspended: this.suspended
    }
  }
}

exports.Worklet = BareKitWorklet

AppState.addEventListener('change', BareKitWorklet.update.bind(BareKitWorklet))
