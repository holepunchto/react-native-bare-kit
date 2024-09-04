#import <React/RCTBridgeModule.h>
#import <React/RCTEventEmitter.h>
#import <React/RCTInvalidating.h>

@interface BareKitModuleWorklet : NSObject

@end

@interface BareKitModule : RCTEventEmitter <RCTBridgeModule, RCTInvalidating>

@end
