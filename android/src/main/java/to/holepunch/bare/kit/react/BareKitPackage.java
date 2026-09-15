package to.holepunch.bare.kit.react;

import com.facebook.react.BaseReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;
import java.util.HashMap;
import java.util.Map;

public class BareKitPackage extends BaseReactPackage {
  public static String NAME = "BareKit";

  static {
    // The library must be loaded from Java so JNI_OnLoad runs and captures the JVM.
    System.loadLibrary("bare-kit");
  }

  @Override
  public NativeModule
  getModule(String name, ReactApplicationContext context) {
    return null;
  }

  @Override
  public ReactModuleInfoProvider
  getReactModuleInfoProvider() {
    return () -> {
      Map<String, ReactModuleInfo> map = new HashMap<>();
      map.put(NAME, new ReactModuleInfo(NAME, NAME, false, false, true, true));
      return map;
    };
  }
}
