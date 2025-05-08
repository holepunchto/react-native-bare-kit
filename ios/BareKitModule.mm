#import <BareKit/BareKit.h>
#import <Foundation/Foundation.h>

#import "BareKitModule.h"

using namespace facebook::react;

@interface BareKitModuleWorklet : NSObject

@end

@implementation BareKitModuleWorklet {
@public
  NSNumber *_id;
  BareKitModule *_module;
  BareWorklet *_worklet;
  BareIPC *_ipc;
}

- (_Nullable instancetype)initWithModule:(BareKitModule *)module
                                      id:(NSNumber *)id
                                filename:(NSString *)filename
                                  source:(NSString *)source
                               arguments:(NSArray<NSString *> *)arguments
                             memoryLimit:(nonnull NSNumber *)memoryLimit
                                  assets:(NSString *)assets {
  self = [super init];

  if (self) {
    _id = id;
    _module = module;

    BareWorkletConfiguration *options = [[BareWorkletConfiguration alloc] init];
    options.memoryLimit = memoryLimit.unsignedIntegerValue;
    options.assets = assets;

    _worklet = [[BareWorklet alloc] initWithConfiguration:options];

    NSData *decoded;

    if (source == nil) {
      decoded = nil;
    } else {
      decoded = [[NSData alloc] initWithBase64EncodedString:source options:0];
    }

    [_worklet start:filename source:decoded arguments:arguments];

    _ipc = [[BareIPC alloc] initWithWorklet:_worklet];
  }

  return self;
}

- (void)_suspend:(int)linger {
  [_worklet suspendWithLinger:linger];
}

- (void)_resume {
  [_worklet resume];
}

- (void)_terminate {
  [_ipc close];
  [_worklet terminate];
}

@end

@implementation BareKitModule {
  int _id;
  NSMutableDictionary<NSNumber *, BareKitModuleWorklet *> *_worklets;
}

RCT_EXPORT_MODULE(BareKit)

- (_Nullable instancetype)init {
  self = [super init];

  if (self) {
    _id = 0;
    _worklets = [[NSMutableDictionary alloc] init];
  }

  return self;
}

- (void)invalidate {
  for (NSNumber *id in _worklets) {
    [_worklets[id] _terminate];
  }

  [_worklets removeAllObjects];
}

- (NSNumber *)start:(NSString *)filename
             source:(NSString *)source
               args:(NSArray *)args
        memoryLimit:(double)memoryLimit
             assets:(NSString *)assets {
  NSNumber *id = @(++_id);

  BareKitModuleWorklet *worklet = [[BareKitModuleWorklet alloc] initWithModule:self
                                                                            id:id
                                                                      filename:filename
                                                                        source:source
                                                                     arguments:args
                                                                   memoryLimit:@(memoryLimit)
                                                                        assets:assets];

  _worklets[id] = worklet;

  return id;
}

- (void)read:(double)id
     resolve:(RCTPromiseResolveBlock)resolve
      reject:(RCTPromiseRejectBlock)reject {
  BareKitModuleWorklet *worklet = _worklets[@(id)];

  if (worklet == nil) return resolve(nil);

  BareIPC *ipc = worklet->_ipc;

  NSData *data = [ipc read];

  if (data) return resolve([data base64EncodedStringWithOptions:0]);

  ipc.readable = ^(BareIPC *ipc) {
    NSData *data = [ipc read];

    if (data == nil) return;

    ipc.readable = nil;

    resolve([data base64EncodedStringWithOptions:0]);
  };
}

- (void)write:(double)id
         data:(NSString *)string
      resolve:(RCTPromiseResolveBlock)resolve
       reject:(RCTPromiseRejectBlock)reject {
  BareKitModuleWorklet *worklet = _worklets[@(id)];

  if (worklet == nil) return resolve(nil);

  BareIPC *ipc = worklet->_ipc;

  NSData *data = [[NSData alloc] initWithBase64EncodedString:string options:0];

  NSInteger written = [ipc write:data];

  if (written == data.length) {
    resolve(nil);
  } else {
    __block NSData *remaining = data;

    remaining = [data subdataWithRange:NSMakeRange(written, remaining.length - written)];

    ipc.writable = ^(BareIPC *ipc) {
      NSInteger written = [ipc write:remaining];

      if (written == remaining.length) {
        ipc.writable = nil;

        resolve(nil);
      } else {
        remaining = [remaining subdataWithRange:NSMakeRange(written, remaining.length - written)];
      }
    };
  }
}

- (void)suspend:(double)id
         linger:(double)linger {
  BareKitModuleWorklet *worklet = _worklets[@(id)];

  if (worklet == nil) return;

  [worklet _suspend:linger];
}

- (void)resume:(double)id {
  BareKitModuleWorklet *worklet = _worklets[@(id)];

  if (worklet == nil) return;

  [worklet _resume];
}

- (void)terminate:(double)id {
  BareKitModuleWorklet *worklet = _worklets[@(id)];

  if (worklet == nil) return;

  [worklet _terminate];

  [_worklets removeObjectForKey:@(id)];
}

- (std::shared_ptr<TurboModule>)getTurboModule:(const ObjCTurboModule::InitParams &)params {
  return std::make_shared<NativeBareKitSpecJSI>(params);
}

@end
