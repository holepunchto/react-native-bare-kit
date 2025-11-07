#pragma once

#include <memory>
#include <optional>

#include "BareKitSpecJSI.h"

namespace facebook::react {

class BareKitModule : public NativeBareKitCxxSpec<BareKitModule> {
public:
  BareKitModule(std::shared_ptr<CallInvoker> jsInvoker);

  jsi::Object
  init(jsi::Runtime &rt, std::optional<jsi::String> id, double memoryLimit, std::optional<jsi::String> assets, jsi::Function on_poll);

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
};

} // namespace facebook::react
