package to.holepunch.bare.kit.react;

import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReadableArray;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.HashMap;
import to.holepunch.bare.kit.IPC;
import to.holepunch.bare.kit.Worklet;
import to.holepunch.bare.kit.react.NativeBareKitSpec;

public class BareKitModule extends NativeBareKitSpec {
  public static String NAME = "BareKit";

  private int id;
  private HashMap<Integer, BareKitModuleWorklet> worklets;

  BareKitModule(ReactApplicationContext context) {
    super(context);

    this.id = 0;
    this.worklets = new HashMap<>();
  }

  @Override
  public String
  getName() {
    return NAME;
  }

  @Override
  public void
  invalidate() {
    super.invalidate();

    for (BareKitModuleWorklet worklet : worklets.values()) {
      worklet.terminate();
    }

    worklets.clear();
  }

  @Override
  public double
  start(String filename, String source, ReadableArray arguments, double memoryLimit, String assets) {
    int id = ++this.id;

    ByteBuffer data;

    if (source == null) {
      data = null;
    } else {
      data = decode(source);
    }

    BareKitModuleWorklet worklet = new BareKitModuleWorklet(this, id, filename, data, arguments.toArrayList().toArray(new String[arguments.size()]), (int) memoryLimit, assets);

    this.worklets.put(id, worklet);

    return id;
  }

  @Override
  public void
  read(double id, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
      return;
    }

    read(worklet.ipc, promise);
  }

  private void
  read(IPC ipc, Promise promise) {
    ByteBuffer data = ipc.read();

    if (data != null) {
      ipc.readable(null);

      promise.resolve(encode(data));
    } else {
      ipc.readable(() -> read(ipc, promise));
    }
  }

  @Override
  public void
  write(double id, String data, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
      return;
    }

    write(worklet.ipc, decode(data), promise);
  }

  private void
  write(IPC ipc, ByteBuffer data, Promise promise) {
    if (ipc.write(data)) {
      ipc.writable(null);

      promise.resolve(null);
    } else {
      ipc.writable(() -> write(ipc, data, promise));
    }
  }

  @Override
  public void
  suspend(double id, double linger) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) return;

    worklet.suspend((int) linger);
  }

  @Override
  public void
  resume(double id) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) return;

    worklet.resume();
  }

  @Override
  public void
  terminate(double id) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) return;

    worklet.terminate();

    worklets.remove(worklet.id);
  }

  private String
  encode(ByteBuffer data) {
    return StandardCharsets.UTF_8.decode(Base64.getEncoder().encode(data)).toString();
  }

  private ByteBuffer
  decode(String data) {
    return ByteBuffer.wrap(Base64.getDecoder().decode(data));
  }

  private static class BareKitModuleWorklet {
    int id;
    BareKitModule module;
    Worklet worklet;
    IPC ipc;

    BareKitModuleWorklet(BareKitModule module, int id, String filename, ByteBuffer source, String[] arguments, int memoryLimit, String assets) {
      this.id = id;
      this.module = module;

      Worklet.Options options = new Worklet.Options().memoryLimit(memoryLimit).assets(assets);

      this.worklet = new Worklet(options);
      this.worklet.start(filename, source, arguments);

      this.ipc = new IPC(this.worklet);
    }

    void
    suspend(int linger) {
      worklet.suspend(linger);
    }

    void
    resume() {
      worklet.resume();
    }

    void
    terminate() {
      ipc.close();
      worklet.terminate();
    }
  }
}
