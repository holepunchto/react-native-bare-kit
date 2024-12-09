package to.holepunch.bare.kit.react;

import com.facebook.react.bridge.BaseJavaModule;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContext;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.ReadableArray;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.HashMap;
import to.holepunch.bare.kit.IPC;
import to.holepunch.bare.kit.Worklet;

public class BareKitModule extends BaseJavaModule implements NativeModule {
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
    return "BareKit";
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

  @ReactMethod
  public void
  start(String filename, String source, ReadableArray arguments, double memoryLimit, String assets, Promise promise) {
    int id = ++this.id;

    ByteBuffer data;

    if (source == null) {
      data = null;
    } else {
      data = decode(source);
    }

    BareKitModuleWorklet worklet = new BareKitModuleWorklet(this, id, filename, data, arguments.toArrayList().toArray(new String[arguments.size()]), (int) memoryLimit, assets);

    this.worklets.put(id, worklet);

    promise.resolve(id);
  }

  @ReactMethod
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

  @ReactMethod
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

  @ReactMethod
  public void
  suspend(double id, double linger, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
      return;
    }

    worklet.suspend((int) linger);

    promise.resolve(null);
  }

  @ReactMethod
  public void
  resume(double id, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
      return;
    }

    worklet.resume();

    promise.resolve(null);
  }

  @ReactMethod
  public void
  terminate(double id, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
      return;
    }

    worklet.terminate();

    worklets.remove(worklet.id);

    promise.resolve(null);
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
