export class Worklet {
  IPC: any;
  RPC: any;

  constructor()

  start(filename: string, source: string): Promise<void>
  terminate(): Promise<void>
}
