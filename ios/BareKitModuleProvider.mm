#import <ReactCommon/CallInvoker.h>
#import <ReactCommon/TurboModule.h>

#import "BareKitModule.h"
#import "BareKitModuleProvider.h"

using namespace facebook::react;

@implementation BareKitModuleProvider

- (std::shared_ptr<TurboModule>)getTurboModule:(const ObjCTurboModule::InitParams &)params {
  return std::make_shared<BareKitModule>(params.jsInvoker);
}

@end
