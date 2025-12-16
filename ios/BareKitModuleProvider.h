#import <Foundation/Foundation.h>

#if __has_include(<ReactCommon/RCTTurboModule.h>)
#import <ReactCommon/RCTTurboModule.h>
#endif

@interface BareKitModuleProvider : NSObject
#if __has_include(<ReactCommon/RCTTurboModule.h>)
<RCTModuleProvider>
#endif

@end
