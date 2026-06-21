#include <memory>
#include <utility>

#include "BareKitModule.h"

namespace facebook::react {

BareKitModule::BareKitModule(std::shared_ptr<CallInvoker> jsInvoker) : BareKitModule(std::move(jsInvoker), nullptr, nullptr) {}

} // namespace facebook::react
