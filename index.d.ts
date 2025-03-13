import type { Duplex } from 'streamx'

export class Worklet {
  IPC: Duplex

  constructor(options?: { memoryLimit?: number; assets?: string })

  start(filename: string, args?: string[]): void
  start(filename: string, source: Uint8Array, args?: string[]): void
  start(filename: string, source: string, args?: string[]): void
  start(
    filename: string,
    source: string,
    encoding: string,
    args?: string[]
  ): void

  suspend(linger?: number): void
  resume(): void
  terminate(): void
}
