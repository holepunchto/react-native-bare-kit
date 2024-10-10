package to.holepunch.bare.kit.react;

import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.BaseJavaModule;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContext;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.HashMap;
import to.holepunch.bare.kit.IPC;
import to.holepunch.bare.kit.Worklet;

public class BareKitModule extends BaseJavaModule implements NativeModule {
  static {
    System.loadLibrary("bare-kit-addons");
  }

  private int id;
  private HashMap<Integer, BareKitModuleWorklet> worklets;

  BareKitModule(ReactApplicationContext context) {
    super(context);

    this.id = 0;
    this.worklets = new HashMap<>();
  }

  @Override
  public String
  getName () {
    return "BareKit";
  }

  @Override
  public void
  invalidate () {
    super.invalidate();

    for (BareKitModuleWorklet worklet : worklets.values()) {
      try {
        worklet.terminate();
      } catch (IOException e) {
        continue;
      }
    }

    worklets.clear();
  }

  private void
  sendEvent (String event, WritableMap params) {
    getReactApplicationContext().getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class).emit(event, params);
  }

  @ReactMethod
  public void
  addListener (String event) {}

  @ReactMethod
  public void
  removeListeners (Integer count) {}

  @ReactMethod
  public void
  start (String filename, String source, ReadableArray arguments, double memoryLimit, String assets, Promise promise) {
    int id = ++this.id;

    ByteBuffer data = source == null ? null : ByteBuffer.wrap(Base64.getDecoder().decode(source));

    BareKitModuleWorklet worklet = new BareKitModuleWorklet(id, this, filename, data, arguments.toArrayList().toArray(new String[arguments.size()]), (int) memoryLimit, assets);

    this.worklets.put(id, worklet);

    promise.resolve(id);
  }

  @ReactMethod
  public void
  write (double id, String data, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
    } else {
      worklet.write(data);

      promise.resolve(null);
    }
  }

  @ReactMethod
  public void
  suspend (double id, double linger, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
    } else {
      worklet.suspend((int) linger);
    }
  }

  @ReactMethod
  public void
  resume (double id, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
    } else {
      worklet.resume();
    }
  }

  @ReactMethod
  public void
  terminate (double id, Promise promise) {
    BareKitModuleWorklet worklet = worklets.get((int) id);

    if (worklet == null) {
      promise.reject("INVALID_ID", new Error("No such worklet found"));
    } else {
      try {
        worklet.terminate();

        worklets.remove(worklet.id);

        promise.resolve(null);
      } catch (IOException e) {
        promise.reject("TERMINATION_FAILED", e);
      }
    }
  }

  private static class BareKitModuleWorklet {
    int id;
    BareKitModule module;
    Worklet worklet;
    IPC ipc;

    BareKitModuleWorklet(int id, BareKitModule module, String filename, ByteBuffer source, String[] arguments, int memoryLimit, String assets) {
      this.id = id;
      this.module = module;

      Worklet.Options options = new Worklet.Options().memoryLimit(memoryLimit).assets(assets);

      this.worklet = new Worklet(options);
      this.worklet.start(filename, source, arguments);

      this.ipc = new IPC(this.worklet);

      read();
    }

    void
    read () {
      ipc.read((data, error) -> {
        if (data == null) return;

        WritableMap params = Arguments.createMap();

        params.putInt("worklet", this.id);

        ByteBuffer encoded = Base64.getEncoder().encode(data);

        params.putString("data", StandardCharsets.UTF_8.decode(encoded).toString());

        module.sendEvent("BareKitIPCData", params);

        read();
      });
    }

    void
    write (String data) {
      ipc.write(ByteBuffer.wrap(Base64.getDecoder().decode(data)));
    }

    void
    suspend (int linger) {
      worklet.suspend(linger);
    }

    void
    resume () {
      worklet.resume();
    }

    void
    terminate () throws IOException {
      ipc.close();

      worklet.terminate();
    }
  }
}
