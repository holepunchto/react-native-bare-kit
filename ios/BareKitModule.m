#import <BareKit/BareKit.h>
#import <Foundation/Foundation.h>

#import "BareKitModule.h"

@implementation BareKitModuleWorklet {
@public
  NSNumber *_id;
  BareKitModule *_module;
  BareWorklet *_worklet;
  BareWorkletConfiguration *_options;
  BareIPC *_ipc;
}

- (_Nullable instancetype)initWithModule:(BareKitModule *)module
                                filename:(NSString *)filename
                                  source:(NSString *)source
                               arguments:(NSArray<NSString *> *)arguments
                             memoryLimit:(nonnull NSNumber *)memoryLimit
                                  assets:(NSString *)assets {
  self = [super init];

  if (self) {
    _id = @((uintptr_t) self);

    _module = module;

    _options = [[BareWorkletConfiguration alloc] init];

    _options.memoryLimit = memoryLimit.unsignedIntegerValue;
    _options.assets = assets;

    _worklet = [[BareWorklet alloc] initWithConfiguration:_options];

    [_worklet start:filename source:[[NSData alloc] initWithBase64EncodedString:source options:0] arguments:arguments];

    _ipc = [[BareIPC alloc] initWithWorklet:_worklet];

    [self _read];
  }

  return self;
}

- (void)_read {
  [_ipc read:^(NSData *data) {
    if (data == nil) return;

    [self->_module sendEventWithName:@"BareKitIPCData"
                                body:@{
                                  @"worklet" : self->_id,
                                  @"data" : [data base64EncodedStringWithOptions:0],
                                }];

    [self _read];
  }];
}

- (void)_write:(NSString *)data {
  [_ipc write:[[NSData alloc] initWithBase64EncodedString:data options:0]];
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
  NSMutableDictionary<NSNumber *, BareKitModuleWorklet *> *_worklets;
}

RCT_EXPORT_MODULE(BareKit)

- (_Nullable instancetype)init {
  self = [super init];

  if (self) {
    _worklets = [[NSMutableDictionary alloc] init];
  }

  return self;
}

- (void)invalidate {
  [super invalidate];

  for (NSNumber *id in _worklets) {
    [_worklets[id] _terminate];
  }

  [_worklets removeAllObjects];
}

- (NSArray<NSString *> *)supportedEvents {
  return @[ @"BareKitIPCData" ];
}

RCT_EXPORT_METHOD(start : (NSString *) filename
                  source : (NSString *) source
                  arguments : (NSArray *) arguments
                  memoryLimit : (nonnull NSNumber *) memoryLimit
                  assets : (NSString *) assets
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = [[BareKitModuleWorklet alloc] initWithModule:self
                                                                      filename:filename
                                                                        source:source
                                                                     arguments:arguments
                                                                   memoryLimit:memoryLimit
                                                                        assets:assets];

  _worklets[worklet->_id] = worklet;

  resolve(worklet->_id);
}

RCT_EXPORT_METHOD(write : (nonnull NSNumber *) id
                  data : (NSString *) data
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = _worklets[id];

  if (worklet == nil) return reject(@"INVALID_ID", @"No such worklet found", nil);

  [worklet _write:data];

  resolve(nil);
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
