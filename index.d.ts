import type { Duplex } from 'streamx'
import type { AppStateStatus } from 'react-native'

export class Worklet {
  IPC: Duplex

  constructor(options?: { memoryLimit?: number; assets?: string })

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
