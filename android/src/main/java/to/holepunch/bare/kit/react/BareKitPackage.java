package to.holepunch.bare.kit.react;

import com.facebook.react.ReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.uimanager.ViewManager;
import java.util.ArrayList;
import java.util.List;

public class BareKitPackage implements ReactPackage {
  @Override
  public List<NativeModule>
  createNativeModules(ReactApplicationContext context) {
    List<NativeModule> modules = new ArrayList<>();

    modules.add(new BareKitModule(context));

    return modules;
  }

  @Override
  public List<ViewManager>
  createViewManagers(ReactApplicationContext context) {
    return new ArrayList<>();
  }
}
