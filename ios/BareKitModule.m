#import <BareKit/BareKit.h>
#import <Foundation/Foundation.h>

#import "BareKitModule.h"

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

- (void)_suspend:(NSNumber *)linger {
  [_worklet suspendWithLinger:linger.intValue];
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

RCT_EXPORT_METHOD(start : (NSString *) filename
                  source : (NSString *) source
                  arguments : (NSArray *) arguments
                  memoryLimit : (nonnull NSNumber *) memoryLimit
                  assets : (NSString *) assets
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  NSNumber *id = @(++_id);

  BareKitModuleWorklet *worklet = [[BareKitModuleWorklet alloc] initWithModule:self
                                                                            id:id
                                                                      filename:filename
                                                                        source:source
                                                                     arguments:arguments
                                                                   memoryLimit:memoryLimit
                                                                        assets:assets];

  _worklets[id] = worklet;

  resolve(id);
}

RCT_EXPORT_METHOD(read : (nonnull NSNumber *) id
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = _worklets[id];

  if (worklet == nil) return reject(@"INVALID_ID", @"No such worklet found", nil);

  NSData *data = [worklet->_ipc read];

  if (data) return resolve([data base64EncodedStringWithOptions:0]);

  worklet->_ipc.readable = ^(BareIPC *ipc) {
    NSData *data = [ipc read];

    if (data == nil) return;

    ipc.readable = nil;

    resolve([data base64EncodedStringWithOptions:0]);
  };
}

RCT_EXPORT_METHOD(write : (nonnull NSNumber *) id
                  data : (NSString *) data
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = _worklets[id];

  if (worklet == nil) return reject(@"INVALID_ID", @"No such worklet found", nil);

  NSData *decoded = [[NSData alloc] initWithBase64EncodedString:data options:0];

  if ([worklet->_ipc write:decoded]) return resolve(nil);

  worklet->_ipc.writable = ^(BareIPC *ipc) {
    if ([ipc write:decoded]) {
      ipc.writable = nil;

      resolve(nil);
    }
  };
}

RCT_EXPORT_METHOD(suspend : (nonnull NSNumber *) id
                  linger : (nonnull NSNumber *) linger
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = _worklets[id];

  if (worklet == nil) return reject(@"INVALID_ID", @"No such worklet found", nil);

  [worklet _suspend:linger];

  resolve(nil);
}

RCT_EXPORT_METHOD(resume : (nonnull NSNumber *) id
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = _worklets[id];

  if (worklet == nil) return reject(@"INVALID_ID", @"No such worklet found", nil);

  [worklet _resume];

  resolve(nil);
}

RCT_EXPORT_METHOD(terminate : (nonnull NSNumber *) id
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = _worklets[id];

  if (worklet == nil) return reject(@"INVALID_ID", @"No such worklet found", nil);

  [worklet _terminate];

  [_worklets removeObjectForKey:id];

  resolve(nil);
}

@end
