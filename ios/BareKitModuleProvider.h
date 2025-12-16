#import <Foundation/Foundation.h>

#if __has_include(<ReactCommon/RCTTurboModule.h>)
#import <ReactCommon/RCTTurboModule.h>
#else
@protocol RCTModuleProvider;
#endif

@interface BareKitModuleProvider : NSObject <RCTModuleProvider>

@end
