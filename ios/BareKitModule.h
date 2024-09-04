#import <React/RCTBridgeModule.h>
#import <React/RCTEventEmitter.h>

@interface BareKitModuleWorklet : NSObject

@end

@interface BareKitModule : RCTEventEmitter <RCTBridgeModule>

@end
