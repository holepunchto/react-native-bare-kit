#pragma once

#include <memory>
#include <optional>

#include "BareKitSpecJSI.h"

extern "C" {
typedef struct bare_worklet_s bare_worklet_t;
}

namespace facebook::react {

typedef int (*BareKitWorkletConfigure)(bare_worklet_t *worklet, void *data);

class BareKitModule : public NativeBareKitCxxSpec<BareKitModule> {
public:
  BareKitModule(std::shared_ptr<CallInvoker> jsInvoker);

  BareKitModule(std::shared_ptr<CallInvoker> jsInvoker, BareKitWorkletConfigure configure, void *configureData);

  jsi::Object
  init(
    jsi::Runtime &rt,
    std::optional<jsi::String> id,
    double memoryLimit,
    std::optional<jsi::String> assets,
    jsi::Function on_terminate,
    jsi::Function on_poll,
    jsi::Function on_suspend,
    jsi::Function on_wakeup,
    jsi::Function on_idle,
    jsi::Function on_resume
  );

  void
  update(jsi::Runtime &rt, jsi::Object handle, bool readable, bool writable);

  void
  startFile(jsi::Runtime &rt, jsi::Object handle, jsi::String filename, jsi::Array args);

  void
  startBytes(jsi::Runtime &rt, jsi::Object handle, jsi::String filename, jsi::Object source, double offset, double length, jsi::Array args);

  void
  startUTF8(jsi::Runtime &rt, jsi::Object handle, jsi::String filename, jsi::String source, jsi::Array args);

  std::optional<jsi::Object>
  read(jsi::Runtime &rt, jsi::Object handle);

  double
  write(jsi::Runtime &rt, jsi::Object handle, jsi::Object data, double offset, double length);

  void
  suspend(jsi::Runtime &rt, jsi::Object handle, double linger);

  void
  resume(jsi::Runtime &rt, jsi::Object handle);

  void
  wakeup(jsi::Runtime &rt, jsi::Object handle, double deadline);

  void
  terminate(jsi::Runtime &rt, jsi::Object handle);

private:
  BareKitWorkletConfigure configure_ = nullptr;
  void *configureData_ = nullptr;
};

} // namespace facebook::react
