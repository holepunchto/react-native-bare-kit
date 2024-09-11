export class Worklet {
  IPC: any;
  RPC: any;

  constructor()

  start(filename: string, source: string): Promise<void>
  suspend(linger?: number): Promise<void>
  resume(): Promise<void>
  terminate(): Promise<void>
}
