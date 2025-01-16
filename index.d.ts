import type { Duplex } from 'streamx'

export class Worklet {
  IPC: Duplex

  constructor(options?: { memoryLimit?: number; assets?: string })

  start(filename: string, args?: string[]): Promise<void>
  start(filename: string, source: Uint8Array, args?: string[]): Promise<void>
  start(filename: string, source: string, args?: string[]): Promise<void>
  start(
    filename: string,
    source: string,
    encoding: string,
    args?: string[]
  ): Promise<void>

  suspend(linger?: number): Promise<void>
  resume(): Promise<void>
  terminate(): Promise<void>
}
