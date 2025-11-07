import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface Spec extends TurboModule {
  init(
    id: string | null,
    memoryLimit: number,
    assets: string | null,
    onpoll: (readable: boolean, writable: boolean) => void
  ): Object

  startFile(handle: Object, filename: string, args: Array<string>): void

  startBytes(
    handle: Object,
    filename: string,
    source: Object,
    offset: number,
    length: number,
    args: Array<string>
  ): void

  startUTF8(
    handle: Object,
    filename: string,
    source: string,
    args: Array<string>
  ): void

  update(handle: Object, readable: boolean, writable: boolean): void

  read(handle: Object): Object | null

  write(handle: Object, data: Object, offset: number, length: number): number

  suspend(handle: Object, linger: number): void

  resume(handle: Object): void

  wakeup(handle: Object, deadline: number): void

  terminate(handle: Object): void
}

export default TurboModuleRegistry.getEnforcing<Spec>('BareKit')
