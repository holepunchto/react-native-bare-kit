import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface Spec extends TurboModule {
  start(
    filename: string,
    source: string,
    args: Array<string>,
    memoryLimit: number,
    assets: string
  ): number

  read(id: number): Promise<string>
  write(id: number, data: string): Promise<void>

  suspend(id: number, linger: number): void
  resume(id: number): void
  terminate(id: number): void
}

export default TurboModuleRegistry.getEnforcing<Spec>('BareKit')
