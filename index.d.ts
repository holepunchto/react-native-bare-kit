import { Duplex } from 'streamx'
import { AppStateStatus } from 'react-native'
import EventEmitter, { EventMap } from 'bare-events'

declare class IPC extends Duplex {
  readonly worklet: Worklet
}

export interface WorkletEvents extends EventMap {
  start: []
  suspend: []
  wakeup: []
  resume: []
  terminate: []
}

export class Worklet extends EventEmitter<WorkletEvents> {
  constructor(options?: { memoryLimit?: number; assets?: string })

  readonly IPC: IPC

  start(filename: string, args?: string[]): void
  start(filename: string, source: Uint8Array, args?: string[]): void
  start(filename: string, source: string, args?: string[]): void

  suspend(linger?: number): void
  resume(): void
  wakeup(deadline?: number): void
  update(state?: AppStateStatus): void
  terminate(): void

  static suspend(linger?: number): void
  static resume(): void
  static wakeup(deadline?: number): void
  static update(state?: AppStateStatus): void
}
